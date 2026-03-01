# VSCode Bridge for Crissy Style Tool

Connects VSCode GitHub Copilot directly to the Crissy Style Tool. Lets you ask Copilot to review, edit, and apply changes to your stylesheets from inside VSCode - and see those changes reflect live in the running style tool.

---

## How it works

```
Browser (Style Tool) <----> bridge.php <----> session.json
                                                  ^
VSCode Copilot <----> MCP Server (Node.js) --------
                          |
                          v
                   style-sheets/*.css  (read + write directly)
```

1. The browser sends its current state (active CSS, active sheet name) to `bridge.php` every 5 seconds.
2. The MCP server exposes tools Copilot calls: read sheets, write sheets, search CSS, read session, list debug tickets.
3. When Copilot writes a stylesheet, it drops a notification file. The browser polls `bridge.php` every 3 seconds, detects the change, and reloads the `<link>` tag - no full page reload needed.

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
| `update_stylesheet` | Write updated CSS back to disk (auto-backups first) |
| `read_active_session` | Read the CSS currently in the editor (requires bridge-sync.js) |
| `search_css` | Search for a selector, property, or value across all sheets |
| `describe_tool` | Get full project context - start here for new features |
| `list_debug_tickets` | Show open error tickets from the debug-tool DB |

---

## File structure

```
vscode-bridge/
  server/
    mcp-server.js       Node.js MCP server (runs via VSCode)
    package.json        Dependencies (@modelcontextprotocol/sdk)
    node_modules/
  api/
    bridge.php          HTTP endpoint for browser <-> MCP session sync
  js/
    bridge-sync.js      Drop into index.php - pushes state, polls changes
  data/
    session.json        Current browser state (auto-written)
    pending-changes.json  Last file changed by Copilot (auto-written)
    .gitignore

.vscode/
  mcp.json             Registers the MCP server with VSCode Copilot
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
