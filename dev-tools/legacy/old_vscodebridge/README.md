# VSCode Bridge for Crissy Style Tool

Connects VSCode GitHub Copilot directly to the Crissy Style Tool. Lets you ask Copilot to review, edit, and apply changes to your stylesheets from inside VSCode - and see those changes reflect live in the running style tool.

---

## Page builder context

The `context/` directory contains reference material for AI-assisted page
builder work:

| File | What it contains |
|---|---|
| `context/section-schema.md` | Full JSON schema for all section types, block types, settings keys, file placement rules, and project color tokens. Read this first before creating any sections. |
| `context/copilot-prompt.md` | Ready-to-paste Copilot prompts for: creating new section templates, adding sections to a specific page, reviewing an existing section, converting stylesheet tokens into section defaults, and building the style tool web page design. |
| `context/design-crissy-style-tool-page.md` | Design spec for the Crissy Style Tool web page: asset registry, layout spec, HTML skeleton, CSS class names, bridge push instructions, and a debug asset checker snippet. |

To use: open `context/copilot-prompt.md`, copy the relevant prompt block,
fill in the bracketed placeholders, and paste into Copilot Chat (Agent mode).

---

## How it works

```
Browser (Style Tool in Tauri) <----> bridge.php <----> session.json
                                |                         ^
                                +---> projects.php <--> projects.db  (SQLite)
                                                          ^
VSCode Copilot <----> MCP Server (Node.js) ----------------
                          |            |
                          |            +--- projects-cli.php --> projects.db
                          v
                   style-sheets/*.css                (CSS editor - read + write)
                   vscode-bridge/data/
                     html-content.json               (HTML editor - write from Copilot)
                     html-ack.json                   (HTML ack from browser)
                     js-content.json                 (JS editor - write from Copilot)
                     js-ack.json                     (JS ack from browser)
                     pending-changes.json            (CSS change signal)
                     changes-ack.json                (CSS ack from browser)
                     wireframe.json                  (wireframe canvas state)
                     wireframe-changes.json          (wireframe change signal)
                     wireframe-ack.json              (wireframe ack from browser)
                     refresh-signal.json             (full-page reload signal)
                     project-update-signal.json      (SQLite save notification)
                     project-update-ack.json         (browser ack for project save)
                     projects.db                     (SQLite project database)
```

1. The browser sends its current state (CSS from CSS editor, HTML from preview, active sheet name) to `bridge.php` every 5 seconds.
2. The MCP server exposes 15 tools that Copilot can call: stylesheet CRUD, HTML/JS push, CSS search, session read, wireframe read/write, project save/load, debug tickets, preview refresh, and describe_tool.
3. When Copilot writes a stylesheet (`update_stylesheet`), it drops a notification file. The browser polls every 3 seconds, detects the change, fetches the updated CSS file, calls `LiveCSS.editor.setCssValue()`, and bumps the `<link>` tag. No full page reload needed.
4. When Copilot calls `push_html_content`, the HTML is written to `html-content.json`. The browser polls every ~3.2 seconds, detects the change, and calls `LiveCSS.editor.setHtmlValue()`, updating the HTML editor and refreshing the preview.
5. When Copilot calls `push_js_content`, the same pattern applies for the JS editor via `LiveCSS.editor.setJsValue()` (polled every ~3.4 seconds).
6. When Copilot calls `save_project`, the HTML + CSS + JS are written to a SQLite database via `projects-cli.php`. A signal file is written. The browser polls every ~3.6 seconds, detects the new save, and auto-loads the project into all 3 editors. The user can also click Load in the app to browse and load saved projects manually. Previous versions are backed up automatically.

---

## Setup

### 1. Install Node dependencies

```bash
cd vscode-bridge/server
npm install
```

### 2. Register with VSCode Copilot

The `.vscode/mcp.json` file at the project root registers the server automatically.

Open this workspace in VSCode, then in Copilot Chat click the tools icon and confirm `crissy-style-tool` is listed.

If it is not listed, open the Command Palette and run:

```
MCP: List Servers
```

Then select `crissy-style-tool` and click Start.

### 3. Add the sync script to the Style Tool

In `index.php`, add this at the bottom of the body, after all other scripts:

```html
<script src="/vscode-bridge/js/bridge-sync.js"></script>
```

This enables:
- Session push (Copilot can see what you are currently editing)
- Change polling (stylesheet updates from Copilot reload live in the browser)

### 4. Point your web server at the project root

Make sure `vscode-bridge/api/bridge.php` is reachable at `/vscode-bridge/api/bridge.php`. This is standard if your document root is the project root.

---

## Using it in Copilot Chat

Once the server is running you can type natural language in Copilot Chat:

```
Review my current stylesheet and tighten the spacing.
```

```
Read dark-neu.css and change all the primary accent colors to a warm orange (#e07b39).
```

```
Search my stylesheets for where border-radius is defined.
```

```
What is currently active in the editor right now?
```

```
List any open critical bugs in the style tool.
```

Copilot will call the appropriate MCP tool, show you what it found or changed, and write any edits directly to the CSS file on disk. The browser picks up the change within 3 seconds.

---

## MCP tools reference

| Tool | What it does |
|---|---|
| `list_stylesheets` | List all CSS files in `style-sheets/` with sizes |
| `read_stylesheet` | Read the full content of a named CSS file |
| `update_stylesheet` | Write updated CSS back to disk; the CSS editor and preview update live within 3 seconds |
| `push_html_content` | Push HTML directly into the HTML editor pane; preview updates live within 3 seconds |
| `push_js_content` | Push JS directly into the JS editor pane; preview updates live within 3 seconds |
| `save_project` | Save HTML + CSS + JS to the SQLite database. The user can click Load in the app to pull the project into all 3 editors. Backs up the previous version automatically. This is the recommended way to push full designs. |
| `list_projects` | List all saved projects in the SQLite database with names, sizes, sources, and timestamps |
| `load_project` | Read a saved project from SQLite. Returns the HTML, CSS, and JS content for review before editing. |
| `read_active_session` | Read the CSS currently in the editor (requires bridge-sync.js) |
| `search_css` | Search for a selector, property, or value across all sheets |
| `describe_tool` | Get full project context and tool inventory. Start here for new features. |
| `list_debug_tickets` | Show open error tickets from the debug-tool DB |
| `read_wireframe` | Read the current wireframe canvas (elements, positions, colors) |
| `update_wireframe` | Push revised wireframe JSON; canvas updates live |
| `refresh_preview` | Tell the browser to do a full page reload. Use after pushing HTML/CSS that needs a re-render. |

---

## File structure

```
vscode-bridge/
  server/
    mcp-server.js            Node.js MCP server (runs via VSCode, 15 tools)
    package.json             Dependencies (@modelcontextprotocol/sdk)
    node_modules/
  api/
    bridge.php               HTTP endpoint for browser <-> MCP session sync
    projects.php             HTTP endpoint for SQLite project CRUD
    projects-cli.php         CLI wrapper so MCP server can call SQLite without HTTP port
  js/
    bridge-sync.js           Drop into index.php - pushes state, polls 7 channels
  data/
    session.json             Current browser state (auto-written)
    pending-changes.json     CSS change signal (auto-written)
    changes-ack.json         CSS change ack from browser
    html-content.json        HTML push from Copilot
    html-ack.json            HTML ack from browser
    js-content.json          JS push from Copilot
    js-ack.json              JS ack from browser
    wireframe.json           Wireframe canvas state
    wireframe-changes.json   Wireframe change signal
    wireframe-ack.json       Wireframe ack from browser
    refresh-signal.json      Full-page reload signal
    project-update-signal.json  SQLite save notification signal
    project-update-ack.json  Project save ack from browser
    projects.db              SQLite database (projects + project_backups tables)
    .gitignore
  context/
    section-schema.md        Full page-builder section JSON schema and rules
    copilot-prompt.md        Ready-to-paste Copilot prompts for section work
    design-crissy-style-tool-page.md  Design spec for the style tool web page

.vscode/
  mcp.json                  Registers the MCP server with VSCode Copilot
```

---

## Backups

Every time Copilot calls `update_stylesheet`, the original file is backed up to:

```
style-sheets/backups/<filename>.<timestamp>.bak
```

You can always restore from there.

---

## Troubleshooting

**Copilot does not show the tools**
- Open `.vscode/mcp.json` and confirm the path to `mcp-server.js` is correct.
- Run `node vscode-bridge/server/mcp-server.js` in a terminal - it should start silently. If it errors, check Node version (>=18 required).

**Session shows "No active session"**
- Make sure `bridge-sync.js` is loaded in `index.php`.
- Check browser DevTools console for `[BridgeSync]` messages.
- Verify `vscode-bridge/api/bridge.php` is reachable (try `curl http://localhost/vscode-bridge/api/bridge.php?action=session`).

**Changes from Copilot are not reloading**
- Check browser console for `[BridgeSync] Copilot changed...` messages.
- If the stylesheet filename does not match any `<link>` tag in the DOM, a full reload may be needed.
- Verify `vscode-bridge/data/` is writable by the web server user.

**MCP server crashes on startup**
- Check `node --version` is 18+.
- Run `cd vscode-bridge/server && npm install` again.
- Look for errors in the VSCode Output panel under "MCP: crissy-style-tool".
