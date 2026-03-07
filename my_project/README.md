# my_project -- Live CSS Editor

Live HTML / CSS / JS editor with an AI agent, CodeMirror editors, VSCode Copilot
bridge, wireframe tool, and AI chat panel.

## Entry Point

```
index.php
```

Serve with PHP built-in server:

```bash
php -S 127.0.0.1:9879 index.php
```

Open `http://127.0.0.1:9879` in any browser.

## Key Directories

| Path | Contents |
|------|----------|
| `css/` | Modular CSS (layout, modal, agent, ai-chat, wireframe, ...) |
| `js/` | Vanilla JS modules under `window.LiveCSS` |
| `js/agent/` | AI agent system (core, ui, run, diff, context, prompts, chat) |
| `js/editor/` | CodeMirror editor sub-modules (goto-css, lint, preview) |
| `data/` | PHP data files: css-properties, property-values, default-content |
| `ai/` | PHP AI backends (Anthropic, OpenAI, DeepSeek) and config |
| `ai/agent/` | Agent PHP handlers (agent.php, run.php, diff.php, outline.php) |
| `style-sheets/` | Seven production CSS design systems used by the AI agent |
| `vscode-bridge/` | VSCode Copilot two-way sync (bridge-sync.js, projects API) |
| `vendor/` | CodeMirror 5.65.16, CSS/JS/HTML linters |
| `scripts/` | Dev helpers (refresh-preview.sh, gen-icon.js, write-readme.js) |

## CSS Theme System

Seven design systems in `style-sheets/`:

| Key | File | Description |
|-----|------|-------------|
| atom-age | atom-age.css | Retro space-age, warm amber |
| clean-system | clean-system.css | Minimal system UI |
| crystal-ui | crystal-ui.css | Glassmorphism |
| dark-neu | dark-neu.css | Dark neumorphism |
| keyboard-ui | keyboard-ui.css | Keyboard / terminal aesthetic |
| neon-grid | neon-grid.css | Cyberpunk neon on dark grid |
| neumorphism | neumorphism.css | Classic soft neumorphism |

`style-sheets/theme_handler.json` is the lightweight routing manifest.
Per-theme detail is in `style-sheets/themes/{key}.json` (loaded on demand).

## VSCode Copilot Bridge

`vscode-bridge/js/bridge-sync.js` polls the PHP API every 4 seconds.
API endpoint: `/my_project/vscode-bridge/api/projects.php`

Projects are stored in `vscode-bridge/data/projects.db` (SQLite).
Project files are mirrored to `vscode-bridge/projects/`.

## AI Config

`ai/config.json` holds API keys (not committed).
`ai/config.php` reads the config and exposes `$aiConfig`.

Provider files: `anthropic.php`, `openai.php`, `deepseek.php`

## push-pull CLI

```bash
php push-pull.php push [name]   # write files to SQLite
php push-pull.php pull [name]   # read from SQLite to files
php push-pull.php list          # list saved projects
```

## License

MIT -- Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
