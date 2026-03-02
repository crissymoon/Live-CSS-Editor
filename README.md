# Crissy's Style Tool

A live CSS/HTML/JavaScript editor with real-time preview, an integrated AI agent, a page builder, a VSCode Copilot bridge, and a built-in admin panel. Runs as a desktop app via Tauri v2 or directly through the PHP built-in server.

**Author:** Crissy Deutsch
**Company:** XcaliburMoon Web Development
**Website:** https://xcaliburmoon.net/
**License:** MIT

---

## Overview

Crissy's Style Tool is a multi-panel coding environment designed for rapid CSS prototyping, page composition, and AI-assisted front-end development. Three resizable CodeMirror editors (CSS, HTML, JS) sit alongside a live preview iframe. All panels are freely draggable, minimizable, and restore their positions between sessions.

The tool ships with a page builder, a section library, a drag-and-drop composer, a VSCode Copilot bridge (MCP server), an AI agent with streaming output, an AI chat interface, a color harmony tool, a wireframe prototyping tool, an admin panel with user management, and a debug/ticket tracking tool.

---

## Features

### Editors

- Three independent CodeMirror 5 editors: CSS, HTML, JavaScript
- Syntax highlighting and linting (CSSLint, JSHint, HTMLHint)
- Code folding with fold gutter
- Per-panel inline search with match count and highlighted results
- Undo/redo buttons in every panel header
- Autosave every 1.5 seconds to localStorage

### Fuzzy Autocomplete

- Context-aware completions: property names before `:`, value keywords after `:`
- JavaScript and HTML completions
- Dropdown flips above the cursor when near the bottom of the viewport
- Arrow key navigation with scroll-into-view
- Capture-phase keyboard interception so arrow keys do not move the cursor while the dropdown is open

### Inline Widgets

- **Color swatches** -- a small diamond appears beside every color value; click it to open a native color picker that updates the value live
- **Size sliders** -- a small diamond appears beside every numeric CSS measurement; drag left or right to adjust the value live
- Supported units: px, em, rem, %, vh, vw, vmin, vmax, pt, pc, ch, ex, cm, mm, in, fr, s, ms, deg, turn
- Bare unitless numbers (flex: 1, z-index: 10) are also detected

### Indent Guides

- Floating settings panel: toggle visibility, color, opacity (0-100%), thickness (1-4 px), style (solid/dashed/dotted), step (every 2/4/8 columns)
- Optional column ruler at a configurable column width with its own color and opacity
- Optional search outline that draws a box around the current CodeMirror search match
- Settings persist to localStorage

### Color Harmony Tool

- Floating panel with seven harmony modes: Complementary, Analogous, Triadic, Split-Comp, Tetradic, Square, Monochromatic
- Base color picker with live hex readout
- Click any swatch to insert the hex value at the cursor in the active editor

### Wireframe Tool

- Full-canvas layout prototyping (1200x900 px canvas) accessible from the View menu
- Add, move, and resize rectangle elements with mouse drag or arrow keys (1 px; Shift+arrow for 10 px)
- Per-element properties: label, position, size, margins, padding, background color, border color, border width, border radius
- Nest elements with a Parent selector; children are constrained inside parent padding bounds
- Anti-overlap enforcement: sibling elements cannot touch or overlap including margin areas
- Horizontal and vertical ruler strips with tick marks every 25 px and labels every 100 px
- Drag from a ruler to create guide lines; drag guides to reposition; double-click to delete
- Save wireframe as JSON, load from JSON, Copy Context (generates a CSS comment block for AI or documentation use)
- Auto-saves to localStorage on every change

### Properties Reference Tool

- Floating searchable panel listing CSS properties, JavaScript APIs, and HTML elements
- Three tabs: CSS / JS / HTML
- Type to filter; accepted values for the selected property appear in the footer
- Insert button pushes the property name into the active editor at the cursor

### Session Management

- Save and load named projects to localStorage
- Session history restore bar on page load (last 10 sessions)
- Named save/load modals
- Reset Layout button restores default panel positions and sizes

---

## AI Agent

The Agent is a floating, resizable window (860x580 px) launched from the header toolbar. Provider, model, task mode, source mode, and active tab all persist to localStorage between sessions.

### Task Modes

- **Build Something New** -- describe a component or layout; the agent selects the best CSS theme and generates complete HTML applied to the editors
- **Request a Change** -- sends current editor content as context; describe an edit and the agent returns a targeted diff
- **Chat** -- open-ended conversation with the selected AI provider

### Source Modes

- **Crissy's Editors** -- uses the live CSS and HTML from the three CodeMirror editors as context
- **Load a File** -- paste or load external file content as context

### Apply Bar

- After generation a diff view shows the before/after for each modified block
- Apply to Editors patches the CSS and HTML editors in place with no copy/paste required

### Neural Animation

- While generating, a canvas overlay shows a drifting neural network with animated signal pulses
- Conversational phrases are displayed over the animation; the phrase pool is seeded from GPT-4o-mini on first use and stored in a local SQLite database

### AI Providers

| Provider | Endpoint | Default Model |
|----------|----------|---------------|
| Anthropic | ai/anthropic.php | claude-opus-4-5 |
| OpenAI | ai/openai.php | gpt-4o |
| DeepSeek | ai/deepseek.php | deepseek-chat |

API keys are stored in `ai/config.json` (not committed to version control).

---

## AI Chat

A persistent chat panel separate from the Agent window for multi-turn conversation with the configured AI provider. Chat history is kept in the session and the interface uses the same provider configuration as the Agent.

---

## Copilot Section Help

A help button in the top-right of the main header opens a panel of quick-copy prompts for using VSCode Copilot to work on page builder sections. Five prompt templates are available:

- **Create a new section template** -- builds a reusable template and saves it to the section library
- **Add a section to a specific page** -- reads the page manifest, writes the file, and patches page.json
- **Preview a section in the browser** -- generates a standalone HTML preview file for a single section
- **Audit a section file for errors** -- validates JSON structure against the full schema
- **Convert active stylesheet to section defaults** -- extracts colors and typography from a stylesheet and applies them to a new section template

Each prompt auto-references `vscode-bridge/context/section-schema.md` so Copilot has full schema context. Copy buttons use the Clipboard API with an execCommand fallback.

---

## CSS Theme System

The `style-sheets/` directory contains seven production-ready CSS design systems used by the AI agent when generating components.

| Key | Prefix | File | Description |
|-----|--------|------|-------------|
| atom-age | aa | atom-age.css | Retro space-age, warm amber tones |
| clean-system | -- | clean-system.css | Minimal system UI for dev tools |
| crystal-ui | cr | crystal-ui.css | Glassmorphism with translucent surfaces |
| dark-neu | dn | dark-neu.css | Dark neumorphism |
| keyboard-ui | kb | keyboard-ui.css | Keyboard/terminal aesthetic |
| neon-grid | ng | neon-grid.css | Cyberpunk neon on dark grid |
| neumorphism | neu | neumorphism.css | Classic soft neumorphism |

Theme metadata is split for performance:

- `style-sheets/theme_handler.json` -- lightweight routing manifest with scoring data (description, best_for, avoid_for, palette_keywords, component_coverage) for all seven themes
- `style-sheets/themes/{key}.json` -- heavy per-theme detail (palette, variables, line counts) loaded on demand only for the winning theme

`style-sheets/parser.php` loads the routing manifest at startup and lazy-loads per-theme files only when building the AI context block. `style-sheets/fuzzy-search.php` searches the manifest for fast component and keyword lookups.

### Theme Randomizer

A JavaScript module (`js/theme-randomizer.js`) lets the agent propose random theme combinations. Configuration is stored in `ai/theme-randomizer.json`.

---

## Page Builder

A JSON-driven page composition tool at `/page-builder/`. Pages are built from modular section files authored by developers or AI.

### How It Works

1. Pages are stored as directories under `page-builder/pages/{page-name}/`
2. Each page has a `page.json` manifest listing section files and their order
3. `build.php` reads the manifest and renders all sections into a complete `index.html`
4. Live edits are stored in `overrides.json` per page; the builder merges them at render time

### Composer (3-panel UI)

Open the composer at `/page-builder/composer.php?page={name}`.

**Library panel (left)** -- browse reusable section templates by type: headers, sections, panels, footers. Tabs filter by type; a search field filters by name. Click a card to add the section to the page.

**Canvas panel (center)** -- lists every section in the page. Drag handles reorder sections. Each row has rename, edit JSON, and remove actions. A status bar shows save/build feedback.

**JSON panel (right)** -- click a section on the canvas to open its raw JSON. Save, format, and cancel actions. Validation errors surface inline.

Top-bar actions: Build Page (write index.html), Preview, Save Order, Back to page index.

### New Page Creation

`create-page.php` scaffolds a new page directory with default header, hero section, footer, and a `page.json` manifest in one step. The header and footer are auto-selected but can be removed in the composer.

### Section Types

| Type | Description |
|------|-------------|
| header | Sticky navigation bar with brand and nav links |
| footer | Copyright line with optional links |
| section | Content block (column or row layout) |
| panel | Two-column layout with main content and sidebar |

### Built-In Section Templates

| File | Description |
|------|-------------|
| headers/basic.json | Sticky nav with brand and four links |
| headers/centered.json | Wide centered brand nav |
| footers/basic.json | Copyright line with Privacy and Terms links |
| footers/minimal.json | Copyright line only |
| sections/hero.json | Centered hero with heading, sub-copy, and CTA button |
| sections/features-3col.json | Three-column feature card grid |
| sections/text-image.json | Two-column text and image row |
| sections/cta-centered.json | Call-to-action band with heading and button |
| panels/with-sidebar.json | Main content area with right sticky sidebar |

### Section Schema (JSON format)

All section files follow a documented schema in `vscode-bridge/context/section-schema.md`. The schema covers block types (heading, text, button, card), all settings key-to-CSS-property mappings, ID uniqueness rules, the page.json manifest format, and the project color palette. New templates dropped into the correct subdirectory under `page-builder/sections/` appear in the library without any registration step.

### Page Index

The page builder index (`/page-builder/index.php`) lists all pages with build status and override count. Includes light/dark mode toggle and links back to the admin dashboard and main style tool.

---

## VSCode Copilot Bridge (MCP Server)

A Model Context Protocol server that connects VSCode GitHub Copilot directly to the style tool. Copilot can read stylesheets, write changes that reflect live in the browser, read the active edit session, search CSS, and check open debug tickets.

### How It Works

```
Browser (Style Tool) <-- bridge.php <-- session.json
                                             ^
VSCode Copilot <-- MCP Server (Node.js) -----+
                        |
                        v
                 style-sheets/*.css  (read + write directly)
```

1. The browser pushes its state (active sheet, CSS content) to `bridge.php` every 5 seconds via `vscode-bridge/js/bridge-sync.js`
2. The MCP server exposes tools that Copilot calls
3. When Copilot writes a stylesheet a notification file is set; the browser polls every 3 seconds and reloads the stylesheet link tag without a full page reload

### MCP Tools

| Tool | Description |
|------|-------------|
| list_stylesheets | List all CSS files in style-sheets/ with sizes |
| read_stylesheet | Read the full content of a named CSS file |
| update_stylesheet | Write updated CSS to disk (auto-backup before writing) |
| read_active_session | Read the CSS currently open in the browser editor |
| search_css | Search all sheets for a selector, property, or value |
| describe_tool | Get full project context for new feature work |
| list_debug_tickets | Show open error tickets from the debug tool database |

### Backups

Every `update_stylesheet` call backs up the original to `style-sheets/backups/{filename}.{timestamp}.bak`.

### Context Folder

`vscode-bridge/context/` contains:

- `section-schema.md` -- complete JSON schema for all section types, block types, settings key mappings, color palette, and file placement rules
- `copilot-prompt.md` -- ready-to-paste Copilot prompts for section creation, page editing, auditing, and stylesheet conversion

---

## Admin Panel

A PHP admin panel at `/pb_admin/` backed by a Go authentication API (`xcm_auth`) on port 9100.

### Features

- Login with session token validation via the Go API
- Dashboard with links to all admin tools
- Light/dark mode across all admin pages (persisted to localStorage)
- Logout with session invalidation

### Admin Tools

| Tool | File | Description |
|------|------|-------------|
| Auth Status | tools/01-auth-status.php | Go auth server status, active session count, token details |
| Sessions | tools/02-sessions.php | List and invalidate active user sessions |
| Audit Log | tools/03-audit-log.php | Login attempts, logouts, token events |
| User Management | tools/04-users.php | Create users, set role (admin/user), deactivate, reactivate |

### Go Auth Server

```bash
./pb_admin/start-auth.sh
```

Runs on port 9100. Handles login, session validation, logout, and user management over a JSON API. `pb_admin/api_proxy.php` proxies requests from the PHP admin pages to the Go API.

---

## TUI Agent

A Python curses terminal UI for AI-assisted file editing, located at `c_tools/tui_agent/`.

```bash
python c_tools/tui_agent/main.py [directory]
```

### How It Works

1. `scanner.py` reads the target directory and lists editable files
2. Select a file, press `p`, and type an instruction
3. Claude Haiku streams a diff via `agent.py`; `fence_clean.py` strips markdown fences; `emoji_clean.py` removes emoji artifacts
4. `merger.py` stages the diff in SQLite via `db.py`
5. Press `a` to approve and apply, `r` to reject

### Convo Commentary

While Haiku processes, `convo.py` runs a GPT-4o mini session in parallel and feeds short developer-slang commentary into the log panel. If the OpenAI API is unavailable it falls back to phrases loaded from `ai/data/phrases.db` so commentary always appears.

### Controls

| Key | Action |
|-----|--------|
| Arrow keys | Navigate file list or scroll focused panel |
| Tab | Cycle focus: Files -> Log -> Diff |
| Enter / Right | Descend into selected directory |
| Backspace / - | Go up one level |
| p | Enter prompt for selected file |
| d | Show diff for selected file |
| a | Approve and apply pending change |
| r | Reject pending change |
| c | Copy focused panel to clipboard |
| v | Toggle view mode (disables mouse for terminal copy) |
| s | Rescan current directory |
| q / Esc | Quit |

### Modules

| File | Role |
|------|------|
| main.py | Curses UI, event loop, animation |
| agent.py | Anthropic Haiku streaming client |
| convo.py | GPT-4o mini live commentary |
| scanner.py | Directory walking and file list |
| merger.py | Diff parsing and apply logic |
| db.py | SQLite staging for pending changes |
| fence_clean.py | Strip markdown code fences from model output |
| emoji_clean.py | Remove emoji artifacts from model output |
| log_util.py | Thread-safe log buffer |

---

## Email Smoke Test

A zero-dependency local email smoke test suite at `email_smoke/`. Spins up a pure-Python SMTP capture server and an HTTP inbox query API to verify send/receive flows without touching any real mail service.

```bash
python email_smoke/run_smoke.py           # run all 8 templates
python email_smoke/run_smoke.py plain_text html_email  # specific tests
python email_smoke/run_smoke.py --list    # print template names
python email_smoke/run_smoke.py --keep    # leave servers running after tests
```

### Servers

| Port | Component | Description |
|------|-----------|-------------|
| 1025 | SMTP capture | Pure-Python socket server; accepts and stores all inbound mail |
| 8025 | HTTP inbox API | Query and clear the in-memory inbox over HTTP |

### HTTP Inbox Endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | / | Server status and message count |
| GET | /messages | List all messages (optional ?to= and ?q= filters) |
| GET | /message/{id} | Single message summary |
| GET | /message/{id}/raw | Raw RFC 2822 source |
| POST | /clear | Delete all messages |

### Test Templates

| Name | Description |
|------|-------------|
| plain_text | Plain text email to single recipient |
| html_email | Multipart email with HTML and plain-text fallback |
| multi_recipient | One message to 3 recipients |
| extra_headers | Reply-To and custom X-headers |
| utf8_content | UTF-8 body with Chinese, Arabic, and accented characters |
| with_attachment | base64-encoded CSV attachment |
| large_body | 64 KB plain-text body stress test |
| auto_responder | HTML auto-responder with Precedence: bulk header |

### Layout

```
email_smoke/
  run_smoke.py              Orchestrator: starts servers, sends all templates, prints results
  email_incoming/
    server.py               Pure-Python SMTP capture server (port 1025)
    inbox.py                Thread-safe in-memory mailbox singleton
    api.py                  HTTP inbox query server (port 8025)
  email_sender/
    sender.py               smtplib wrapper with reconnect retry
    templates.py            8 test message factories
```

---

## Prompt Injection Guard

A fine-tuned DistilBERT classifier at `prompt_inj_guard/` that flags incoming text as `clean`, `spam`, or `prompt_injection`. Intended as a guard layer for AI-assisted features before user input reaches a model.

- Model: DistilBERT-base (6 layers, 768 dim, 67M params, float32)
- Weights: ~255 MB stored locally in `prompt_inj_guard/model/spam_injection_model/` (gitignored)

### REST API

```bash
cd prompt_inj_guard/api
pip install -r requirements.txt
python server.py --port 8765
```

| Method | Path | Description |
|--------|------|-------------|
| GET | /health | Model load status and load time |
| POST | /classify | Classify a text string; returns label and confidence scores |

### Layout

```
prompt_inj_guard/
  api/
    server.py               Flask REST endpoint
    requirements.txt
  model/
    guard_classifier.py     GuardClassifier class
    requirements_inference.txt
    spam_injection_model/   GITIGNORED -- model weights
      final/
        model.safetensors
        tokenizer.json
        config.json
        label_map.json
```

---

## Database Browser

A GTK+3 SQLite browser written in C at `db-browser/`. Built as a local devtool for inspecting SQLite databases used by the debug tool, the TUI agent, and the phrase store.

```bash
cd db-browser
make
./build/db-browser
```

Requires: GTK+3, libsqlite3, gcc.

### Features

- Table browser: list tables, browse rows with pagination
- Query editor: write and run arbitrary SQL; results in a scrollable grid
- Data browser: inspect raw cell values including blobs
- Recent files dropdown (last 10 databases)
- Status bar with row/column count feedback

---

## Debug Tool

A standalone error ticket tracking tool at `/debug-tool/`. Used to log, triage, and resolve bugs during development.

- AI-assisted error analysis (`debug-tool/ai/analyze.php`)
- REST API for submitting and reading tickets (`debug-tool/api/`)
- SQLite ticket database
- CLI tools for querying tickets
- The VSCode Bridge MCP server exposes `list_debug_tickets` so Copilot can see open tickets when working on fixes

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Desktop shell | Tauri v2 (Rust) |
| Backend | PHP 8.x built-in server |
| Auth API | Go (xcm_auth) |
| Editors | CodeMirror 5.65.16 (local vendor) |
| Linters | CSSLint 1.0.5, JSHint 2.13.6, HTMLHint 0.16.3 |
| AI streaming | Server-Sent Events via PHP |
| Phrase storage | SQLite via PHP |
| MCP server | Node.js 18+ with @modelcontextprotocol/sdk |
| JS modules | Vanilla ES5/ES6, namespaced under window.LiveCSS |
| CSS | Custom dark theme, no framework |
| TUI agent | Python 3, curses, Anthropic Haiku, OpenAI GPT-4o mini |
| Email smoke | Python 3 stdlib only (socket, smtplib, http.server, email.mime) |
| Prompt guard | Python 3, DistilBERT (transformers + torch), Flask |
| DB browser | C, GTK+3, libsqlite3 |

---

## Project Structure

```
live-css/
  index.php                    Main HTML shell and PHP data bridge
  style.css                    Global styles
  style-context.txt            Active project color/typography tokens

  css/                         Modular CSS
    base.css
    layout.css
    agent.css
    ai-chat.css
    fuzzy.css
    wireframe.css
    color-tools.css
    header.css
    info-bar.css
    modal.css
    native-bridge.css
    responsive.css
    scrollbar.css

  js/                          Modular JavaScript
    app.js                     Boot and wiring
    agent.js                   Agent entry point
    agent/
      agent-core.js
      agent-ui.js
      agent-run.js
      agent-diff.js
      agent-context.js
      agent-prompts.js
      agent-chat.js
      agent-window.js
      agent-neural.js
    ai-chat.js
    editor.js
    fuzzy.js
    color-swatch.js
    size-slider.js
    color-harmony.js
    wireframe.js
    gutter.js
    editor-search.js
    indent-guide.js
    storage.js
    modal-save.js
    modal-load.js
    property-lookup.js
    theme-randomizer.js
    native-bridge.js
    cdn-loader.js
    utils.js

  ai/                          PHP AI backend
    config.json                API keys (not committed)
    config.php
    anthropic.php
    openai.php
    deepseek.php
    phrases.php
    theme-randomizer.json
    data/
      phrases.db
    agent/
      agent.php
      run.php
      diff.php
      outline.php
      db.php
      prompts.json
      context/
        context.php

  style-sheets/                CSS design systems
    theme_handler.json
    themes/
    parser.php
    fuzzy-search.php
    rules.json
    atom-age.css
    clean-system.css
    crystal-ui.css
    dark-neu.css
    keyboard-ui.css
    neon-grid.css
    neumorphism.css
    backups/

  page-builder/                Page composition tool
    index.php
    composer.php
    build.php
    create-page.php
    section-api.php
    section-library.php
    css/
      pb-theme.css
      pb-composer.css
    js/
      pb-composer.js
    sections/
      headers/
      footers/
      sections/
      panels/
    pages/

  vscode-bridge/               VSCode Copilot MCP integration
    server/
      mcp-server.js
      package.json
    api/
      bridge.php
    js/
      bridge-sync.js
    data/
      session.json
      pending-changes.json
    context/
      section-schema.md
      copilot-prompt.md

  pb_admin/                    Admin panel
    index.php
    login.php
    dashboard.php
    auth.php
    api_proxy.php
    router.php
    start-auth.sh
    tools/
      01-auth-status.php
      02-sessions.php
      03-audit-log.php
      04-users.php

  debug-tool/                  Error ticket tracker
    ai/
    api/
    cli/
    db/
    js/

  c_tools/                     Local developer tools (Python)
    tui_agent/                 Curses TUI AI coding agent
      main.py                  Curses UI and event loop
      agent.py                 Anthropic Haiku streaming client
      convo.py                 GPT-4o mini live commentary
      scanner.py               Directory walker
      merger.py                Diff parser and apply logic
      db.py                    SQLite staging for pending changes
      fence_clean.py           Strip markdown fences from model output
      emoji_clean.py           Remove emoji artifacts
      log_util.py              Thread-safe log buffer

  email_smoke/                 Local SMTP smoke test suite (Python)
    run_smoke.py               Orchestrator
    email_incoming/
      server.py                SMTP capture server (port 1025)
      inbox.py                 In-memory mailbox singleton
      api.py                   HTTP inbox API (port 8025)
    email_sender/
      sender.py                smtplib wrapper
      templates.py             8 test message factories

  prompt_inj_guard/            Prompt injection / spam classifier
    api/
      server.py                Flask REST API (port 8765)
    model/
      guard_classifier.py      GuardClassifier (DistilBERT)
      spam_injection_model/    GITIGNORED -- model weights

  db-browser/                  GTK+3 SQLite browser (C)
    main.c
    core/
    ui/
    Makefile

  data/
    css-properties.php
    property-values.php
    default-content.php

  vendor/
    codemirror/
    linters/

  src-tauri/                   Tauri desktop app shell (Rust)
  scripts/
    copy-www.js
    split-learn-json.js
    gen-icon.js
    write-readme.js
    write-neural.js
    refresh-preview.sh
```

---

## Running Locally

### PHP server (browser only)

```bash
php -S 127.0.0.1:8080 index.php
```

Open http://127.0.0.1:8080.

### PHP server with admin panel and page builder

```bash
php -S 127.0.0.1:8080 pb_admin/router.php
```

Serves the full app including `/pb_admin/` and `/page-builder/` on the same port.

### Go auth server (required for admin panel)

```bash
./pb_admin/start-auth.sh
```

Runs on port 9100.

### VSCode Copilot Bridge

```bash
cd vscode-bridge/server && npm install
```

The MCP server starts automatically when VSCode opens the workspace (configured in `.vscode/mcp.json`). To start manually:

```bash
node vscode-bridge/server/mcp-server.js
```

---

## Building the Desktop App

```bash
node scripts/copy-www.js
npm run tauri build
```

Requires Rust, Cargo, and the Tauri CLI. `copy-www.js` must run before every build to sync web assets into `src-tauri/www/`.

For live development:

```bash
npx tauri dev
```

---

## License

MIT License -- see [LICENSE](LICENSE) for full text.

Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
https://xcaliburmoon.net/
