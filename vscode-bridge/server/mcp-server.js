/**
 * vscode-bridge/server/mcp-server.js
 * MCP server - exposes Crissy Style Tool stylesheets to VSCode Copilot.
 *
 * Tools for Copilot:
 *   list_stylesheets      - list all CSS files in style-sheets/
 *   read_stylesheet       - read the full CSS of one file
 *   update_stylesheet     - write CSS changes back (reflected live in the tool)
 *   read_active_session   - read the current session state pushed from the browser
 *   search_css            - search for a selector / property across all sheets
 *   describe_tool         - project context for Copilot (what this app is)
 *   list_debug_tickets    - read open error tickets from the debug-tool DB
 *   refresh_preview       - tell the browser to do a full page reload
 *
 * Console/stderr logging is used throughout so Copilot can see diagnostics.
 */

import { McpServer }          from '@modelcontextprotocol/sdk/server/mcp.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import { z }                  from 'zod';
import fs                     from 'fs';
import path                   from 'path';
import { fileURLToPath }      from 'url';
import { execSync }           from 'child_process';

const __dirname   = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, '..', '..');
const SHEETS_DIR   = path.join(PROJECT_ROOT, 'style-sheets');
const SESSION_FILE = path.join(__dirname, '..', 'data', 'session.json');
const CHANGES_FILE = path.join(__dirname, '..', 'data', 'pending-changes.json');
const REFRESH_FILE = path.join(__dirname, '..', 'data', 'refresh-signal.json');
const DEBUG_DB     = path.join(PROJECT_ROOT, 'debug-tool', 'db', 'errors.db');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Safe JSON read - returns null and logs on failure */
function readJson(filePath) {
    try {
        if (!fs.existsSync(filePath)) return null;
        return JSON.parse(fs.readFileSync(filePath, 'utf8'));
    } catch (err) {
        console.error(`[MCP] readJson failed for ${filePath}: ${err.message}`);
        return null;
    }
}

/** Ensure a directory exists */
function ensureDir(dir) {
    try {
        if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });
    } catch (err) {
        console.error(`[MCP] ensureDir failed for ${dir}: ${err.message}`);
    }
}

/** Return all .css files in SHEETS_DIR */
function listCssFiles() {
    try {
        return fs.readdirSync(SHEETS_DIR)
            .filter(f => f.toLowerCase().endsWith('.css'))
            .sort();
    } catch (err) {
        console.error(`[MCP] listCssFiles error: ${err.message}`);
        return [];
    }
}

/** Read a CSS file by its filename (basename only, no path traversal) */
function readSheet(name) {
    const safe = path.basename(name);
    const full = path.join(SHEETS_DIR, safe);
    if (!full.startsWith(SHEETS_DIR)) {
        console.error(`[MCP] readSheet: path traversal attempt blocked for "${name}"`);
        throw new Error('Invalid stylesheet name.');
    }
    if (!fs.existsSync(full)) throw new Error(`Stylesheet not found: ${safe}`);
    return fs.readFileSync(full, 'utf8');
}

/** Atomically write a CSS file - backs up the previous version first */
function writeSheet(name, css) {
    const safe    = path.basename(name);
    const full    = path.join(SHEETS_DIR, safe);
    if (!full.startsWith(SHEETS_DIR)) {
        console.error(`[MCP] writeSheet: path traversal attempt blocked for "${name}"`);
        throw new Error('Invalid stylesheet name.');
    }

    // Backup
    if (fs.existsSync(full)) {
        const backupDir = path.join(SHEETS_DIR, 'backups');
        ensureDir(backupDir);
        const ts     = new Date().toISOString().replace(/[:.]/g, '-');
        const backup = path.join(backupDir, `${safe}.${ts}.bak`);
        try {
            fs.copyFileSync(full, backup);
            console.error(`[MCP] writeSheet: backed up to ${backup}`);
        } catch (err) {
            console.error(`[MCP] writeSheet: backup failed (continuing): ${err.message}`);
        }
    }

    fs.writeFileSync(full, css, 'utf8');
    console.error(`[MCP] writeSheet: wrote ${css.length} chars to ${full}`);

    // Write a pending-change notification so the browser client can pick it up
    try {
        ensureDir(path.dirname(CHANGES_FILE));
        fs.writeFileSync(CHANGES_FILE, JSON.stringify({
            file:      safe,
            updatedAt: new Date().toISOString(),
            by:        'vscode-copilot',
        }), 'utf8');
    } catch (err) {
        console.error(`[MCP] writeSheet: could not write pending-changes.json: ${err.message}`);
    }
}

/** Run debug-tool CLI and return its output (ANSI stripped) */
function runDebugCli(args) {
    const cliPath = path.join(PROJECT_ROOT, 'debug-tool', 'cli', 'debug-cli.php');
    const cmd     = `php ${cliPath} ${args}`;
    try {
        const out = execSync(cmd, { encoding: 'utf8', timeout: 8000 });
        return out.replace(/\x1B\[[0-9;]*m/g, '');
    } catch (err) {
        console.error(`[MCP] runDebugCli error for cmd "${cmd}": ${err.message}`);
        return null;
    }
}

// ---------------------------------------------------------------------------
// MCP Server setup
// ---------------------------------------------------------------------------

const server = new McpServer({
    name:    'crissy-style-tool',
    version: '1.0.0',
});

// ---------------------------------------------------------------------------
// Tool: list_stylesheets
// ---------------------------------------------------------------------------
server.tool(
    'list_stylesheets',
    'List all CSS stylesheets available in Crissy Style Tool with their file size and path.',
    {},
    async () => {
        try {
            const files = listCssFiles();
            if (files.length === 0) {
                return { content: [{ type: 'text', text: 'No CSS files found in style-sheets/.' }] };
            }

            const rows = files.map(f => {
                try {
                    const stat  = fs.statSync(path.join(SHEETS_DIR, f));
                    const bytes = stat.size;
                    const kb    = (bytes / 1024).toFixed(1);
                    return `${f}  (${kb} KB)`;
                } catch {
                    return `${f}  (size unknown)`;
                }
            });

            const text = `${files.length} stylesheets in style-sheets/:\n\n` + rows.join('\n');
            console.error(`[MCP] list_stylesheets: returned ${files.length} files`);
            return { content: [{ type: 'text', text }] };
        } catch (err) {
            console.error(`[MCP] list_stylesheets error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error listing stylesheets: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: read_stylesheet
// ---------------------------------------------------------------------------
server.tool(
    'read_stylesheet',
    'Read the full CSS content of a stylesheet from Crissy Style Tool. Pass the filename (e.g. "atom-age.css" or "dark-neu.css").',
    { name: z.string().min(1).describe('Stylesheet filename, e.g. "atom-age.css"') },
    async ({ name }) => {
        try {
            const css  = readSheet(name);
            const safe = path.basename(name);
            console.error(`[MCP] read_stylesheet: ${safe} (${css.length} chars)`);
            return { content: [{ type: 'text', text: `/* ${safe} */\n\n${css}` }] };
        } catch (err) {
            console.error(`[MCP] read_stylesheet error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error reading stylesheet: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: update_stylesheet
// ---------------------------------------------------------------------------
server.tool(
    'update_stylesheet',
    'Write updated CSS content to a stylesheet in Crissy Style Tool. The change reflects immediately in the app (browser reloads the file). A backup is created automatically before writing.',
    {
        name: z.string().min(1).describe('Stylesheet filename, e.g. "dark-neu.css"'),
        css:  z.string().min(1).describe('The full updated CSS content to write.'),
    },
    async ({ name, css }) => {
        try {
            const safe = path.basename(name);
            writeSheet(safe, css);
            console.error(`[MCP] update_stylesheet: wrote "${safe}" (${css.length} chars)`);
            return {
                content: [{
                    type: 'text',
                    text: `Updated "${safe}" successfully (${css.length} chars written).\n\nThe style tool will pick up this change when the stylesheet is reloaded. If the sync client is active in the browser it will reload automatically within 3 seconds.`,
                }],
            };
        } catch (err) {
            console.error(`[MCP] update_stylesheet error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error updating stylesheet: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: read_active_session
// ---------------------------------------------------------------------------
server.tool(
    'read_active_session',
    'Read the current active editing session from Crissy Style Tool - includes the CSS being edited, the active stylesheet name, and when it was last synced from the browser.',
    {},
    async () => {
        try {
            const session = readJson(SESSION_FILE);
            if (!session) {
                return {
                    content: [{
                        type: 'text',
                        text: 'No active session found. Open the Style Tool in a browser and the session will sync here automatically (requires the vscode-bridge/js/bridge-sync.js snippet to be loaded in the app).',
                    }],
                };
            }

            const lines = [
                `Active stylesheet : ${session.activeSheet  || 'unknown'}`,
                `Last synced       : ${session.syncedAt     || 'unknown'}`,
                `Source            : ${session.source       || 'browser'}`,
                '',
                '--- Current CSS being edited ---',
                session.css || '(empty)',
            ];

            if (session.html) {
                lines.push('', '--- Current HTML preview ---', session.html);
            }

            console.error(`[MCP] read_active_session: session found, synced at ${session.syncedAt}`);
            return { content: [{ type: 'text', text: lines.join('\n') }] };
        } catch (err) {
            console.error(`[MCP] read_active_session error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error reading session: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: search_css
// ---------------------------------------------------------------------------
server.tool(
    'search_css',
    'Search for a CSS selector, property, or value across all stylesheets in Crissy Style Tool. Returns matching lines with file and line number context.',
    {
        query:     z.string().min(1).describe('Search string - a selector, property name, or value to find.'),
        sheet:     z.string().optional().describe('Limit search to one stylesheet filename. If omitted, searches all sheets.'),
        maxResults: z.number().min(1).max(200).optional().describe('Max results to return (default 40).'),
    },
    async ({ query, sheet, maxResults = 40 }) => {
        try {
            const files = sheet ? [path.basename(sheet)] : listCssFiles();
            const results = [];
            const lq = query.toLowerCase();

            for (const file of files) {
                try {
                    const css   = readSheet(file);
                    const lines = css.split('\n');
                    lines.forEach((line, i) => {
                        if (line.toLowerCase().includes(lq)) {
                            results.push(`${file}:${i + 1}  ${line.trim()}`);
                        }
                    });
                } catch (e) {
                    console.error(`[MCP] search_css: skip ${file}: ${e.message}`);
                }

                if (results.length >= maxResults) break;
            }

            if (results.length === 0) {
                return { content: [{ type: 'text', text: `No matches found for "${query}".` }] };
            }

            const truncated = results.slice(0, maxResults);
            const suffix    = results.length > maxResults ? `\n... (${results.length - maxResults} more results omitted)` : '';
            const text      = `${truncated.length} match(es) for "${query}":\n\n` + truncated.join('\n') + suffix;

            console.error(`[MCP] search_css: "${query}" -> ${results.length} results`);
            return { content: [{ type: 'text', text }] };
        } catch (err) {
            console.error(`[MCP] search_css error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error during search: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: describe_tool
// ---------------------------------------------------------------------------
server.tool(
    'describe_tool',
    'Get a full description of the Crissy Style Tool project structure, what it does, and how it is organized. Use this first when adding features or reviewing the codebase.',
    {},
    async () => {
        try {
            const sheets = listCssFiles();
            const text = `
Crissy Style Tool - Live CSS Editor
=====================================
Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
https://xcaliburmoon.net/

WHAT IT IS:
  A browser-based live CSS editor / theme designer. Users can edit CSS in a
  CodeMirror editor, preview results in real time, and switch between multiple
  named themes/stylesheets.

PROJECT ROOT: ${PROJECT_ROOT}

KEY DIRECTORIES:
  style-sheets/       All theme CSS files (the main "stylesheets" the user edits)
  css/                App UI CSS (editor chrome, layout, modals, etc.)
  js/                 Frontend JS (editor, storage, fuzzy search, color tools, etc.)
  ai/                 PHP AI providers (OpenAI, Anthropic, DeepSeek) + agent
  data/               PHP data providers (CSS properties, stylesheets list, etc.)
  debug-tool/         SQLite3 error tracking tool (tickets, AI analysis, CLI)
  vscode-bridge/      This bridge - connects Copilot to the style tool
  vendor/             CodeMirror and linters

AVAILABLE STYLESHEETS (${sheets.length}):
${sheets.map(s => '  ' + s).join('\n')}

MAIN ENTRY POINTS:
  index.php           Main app (PHP, loads all CSS/JS, serves the editor UI)
  js/app.js           JS app init and core wiring
  js/editor.js        CodeMirror editor integration
  js/storage.js       localStorage project persistence
  ai/config.php       AI API keys and provider selection

HOW STYLESHEETS WORK:
  - CSS files in style-sheets/ are the themes. Each is a standalone CSS file.
  - The user picks a theme; it loads via <link> or is injected into the editor.
  - Edits in the CodeMirror editor are NOT automatically saved to disk.
    They exist in localStorage (autosave) until the user explicitly saves.
  - To change a stylesheet permanently: edit the .css file in style-sheets/.
    This bridge's update_stylesheet tool does exactly that.

HOW THE VSCODE BRIDGE WORKS:
  1. read_active_session   - see what is currently in the editor
  2. read_stylesheet       - read any named theme file
  3. update_stylesheet     - write Copilot's changes back to disk
  4. search_css            - find where a selector or property is defined
  5. list_debug_tickets    - see open error tickets

TYPICAL WORKFLOW:
  User: "Review my dark-neu.css and tighten the spacing"
  Copilot: read_stylesheet("dark-neu.css") -> edits -> update_stylesheet(...)

  User: "What is the current background color?"
  Copilot: read_active_session() or search_css("background")

  User: "There is a bug with the scrollbar"
  Copilot: list_debug_tickets() -> analyzes -> suggests fix
`.trim();

            console.error('[MCP] describe_tool called');
            return { content: [{ type: 'text', text }] };
        } catch (err) {
            console.error(`[MCP] describe_tool error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: list_debug_tickets
// ---------------------------------------------------------------------------
server.tool(
    'list_debug_tickets',
    'List open error tickets from the Crissy Style Tool debug database. Useful for reviewing what bugs exist before making changes.',
    {
        level:  z.enum(['critical', 'high', 'medium', 'low', 'info', 'all']).optional().describe('Filter by level (default all open)'),
        limit:  z.number().min(1).max(50).optional().describe('Max tickets to return (default 20)'),
    },
    async ({ level = 'all', limit = 20 }) => {
        try {
            const levelFlag = level !== 'all' ? `--level=${level}` : '';
            const args      = ['list', '--status=open', levelFlag, `--limit=${limit}`].filter(Boolean).join(' ');
            console.error(`[MCP] list_debug_tickets: php debug-cli.php ${args}`);
            const out = runDebugCli(args);
            return { content: [{ type: 'text', text: out || 'No open tickets found.' }] };
        } catch (err) {
            console.error(`[MCP] list_debug_tickets error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error fetching tickets: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Tool: refresh_preview
// ---------------------------------------------------------------------------
server.tool(
    'refresh_preview',
    'Tell the Crissy Style Tool browser window to do a full page reload. Use this after pushing HTML/CSS changes that require the HTML to be re-rendered (e.g. after adding new elements that are not in the current live session).',
    {},
    async () => {
        try {
            const dataDir = path.join(__dirname, '..', 'data');
            ensureDir(dataDir);
            const ts      = new Date().toISOString().replace('T', ' ').slice(0, 19);
            const payload = JSON.stringify({ refresh: true, requestedAt: ts }, null, 2);
            fs.writeFileSync(REFRESH_FILE, payload, 'utf8');
            console.error(`[MCP] refresh_preview: signal written at ${ts}`);
            return { content: [{ type: 'text', text: `Refresh signal sent at ${ts}. The browser will reload within 2 seconds if the bridge is ON.` }] };
        } catch (err) {
            console.error(`[MCP] refresh_preview error: ${err.message}`);
            return { content: [{ type: 'text', text: `Error sending refresh signal: ${err.message}` }], isError: true };
        }
    }
);

// ---------------------------------------------------------------------------
// Start
// ---------------------------------------------------------------------------
try {
    const transport = new StdioServerTransport();
    await server.connect(transport);
    console.error('[MCP] Crissy Style Tool MCP server started (stdio)');
} catch (err) {
    console.error('[MCP] Failed to start server:', err.message);
    process.exit(1);
}
