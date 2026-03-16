# Live CSS Editor

**Author:** Crissy Deutsch  
**Company:** XcaliburMoon Web Development  
**Website:** https://xcaliburmoon.net/  
**License:** MIT

A live HTML / CSS / JS editor with an AI agent, VSCode Copilot bridge, page builder, Go auth API, and a cross-platform native browser shell. The style tool, page builder, auth system, dev utilities, and desktop app packaging pipeline all run locally with no cloud dependency for the core workflow.

## Quick Start

```bash
# Start PHP server (style tool only)
cd my_project
php -S 127.0.0.1:9879 index.php

# Start full stack (nginx + auth + browser)
bash server/start.sh
bash page-builder/pb_admin/start-auth.sh
bash imgui-browser/run.sh

# Push to GitHub (auto-updates this README first)
bash push.sh

# Windows GitHub CLI push flow
powershell -ExecutionPolicy Bypass -File .\push-win.ps1
```

## Directory Map

| Folder | Description | README |
|--------|-------------|--------|
| `my_project/` | Live HTML/CSS/JS editor with AI agent and VSCode bridge | [README](my_project/README.md) |
| `page-builder/` | JSON-driven page composer + admin panel + Go auth | [README](page-builder/README.md) |
| `dev-tools/` | Dev utilities: browser, debug tracker, TUI agent, tools | [README](dev-tools/README.md) |
| `imgui-browser/` | Cross-platform native browser shell (C++ / Dear ImGui) -- packaging target | [README](imgui-browser/README.md) |
| `server/` | nginx + PHP-FPM configuration and startup scripts | [README](server/README.md) |
| `db_bridge/` | Shared SQLite bridge API (PHP + JS client) | [README](db_bridge/README.md) |

## Project Structure

```
live-css/
  my_project/  -- Live HTML/CSS/JS editor with AI agent and VSCode bridge
  page-builder/  -- JSON-driven page composer + admin panel + Go auth
  dev-tools/  -- Dev utilities: browser, debug tracker, TUI agent, tools
  imgui-browser/  -- Cross-platform native browser shell (C++ / Dear ImGui) -- packaging target
  server/  -- nginx + PHP-FPM configuration and startup scripts
  db_bridge/  -- Shared SQLite bridge API (PHP + JS client)
  crissys-notes.md
  launcher.json
  make_readme.py
  push-win.ps1
  push.sh
  smoke-tools.json
```

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Style tool backend | PHP 8.x built-in server |
| Desktop shell | C++ / Dear ImGui / WKWebView -- cross-platform packaging target |
| Dev browser (local only) | Python 3 + PyQt6 / WKWebView via pyobjc |
| Auth API | Go (xcm_auth, port 9100) |
| Editors | CodeMirror 5.65.16 (local vendor) |
| AI streaming | Server-Sent Events via PHP |
| MCP server | Node.js 18+ with @modelcontextprotocol/sdk |
| TUI agent | Python 3, curses, Anthropic Haiku, OpenAI GPT-4o mini |
| DB browser | C, GTK+3, libsqlite3 |
| Mood check | Python 3, EMI bio-inspired engine, 370K-word lexicon |
| Prompt guard | Python 3, DistilBERT (transformers + torch), Flask |
| Email smoke | Python 3 stdlib only |
| Phrase storage | SQLite via PHP |
| JS modules | Vanilla ES5/ES6 under window.LiveCSS |

## License

MIT License -- see [LICENSE](LICENSE) for full text.

Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development

---

*README last generated: 2026-03-16  00:24:03*
