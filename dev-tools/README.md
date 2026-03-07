# dev-tools

Collection of local developer utilities for the Live CSS Editor project.

## Contents

| Directory | Description |
|-----------|-------------|
| `dev-browser/` | Python 3 + PyQt6 / WKWebView custom desktop browser |
| `debug-tool/` | Error ticket tracker with AI analysis, REST API, and SQLite |
| `agent-flow/` | Visual agent workflow UI (PHP, port 9090) |
| `c_tools/` | Python dev tools (TUI agent, PDF convert, outliner) |
| `c_tools/tui_agent/` | Curses TUI AI coding agent (Haiku + GPT-4o mini) |
| `code-review/` | Code audit scripts (lines_count, security_ck, py_audit, god_funcs, search) |
| `db-browser/` | GTK+3 SQLite browser written in C |
| `email_smoke/` | Zero-dependency local SMTP smoke test suite |
| `legacy/` | Archived Tauri v2 shell, launcher.sh, vscode-bridge-server |
| `zyx_planning_and_visuals/` | Planning notes and HTML report generator |
| `editz/` | Editor utilities |

## dev-browser

Custom desktop browser replacing the former Tauri shell.

```bash
cd dev-browser
python webbrowse.py
python webbrowse.py --url https://localhost:8443
```

Two engines per tab: WKWebView (macOS native, DRM) and QtWebEngine (Chromium).

## debug-tool

Error ticket tracker. Exposes `list_debug_tickets` via the VSCode MCP bridge.

```
debug-tool/
  api/        REST endpoints
  cli/        Command-line ticket queries
  db/         SQLite database (errors.db)
  js/         Front-end
  ai/         AI-assisted error analysis
```

## TUI Agent

```bash
python dev-tools/c_tools/tui_agent/main.py [directory]
```

Select a file, press `p` to write an instruction. Haiku streams a diff.
Press `a` to apply, `r` to reject.

Press `P` for project mode: scans directory, GPT-4o mini plans, Haiku applies.

## DB Browser

```bash
cd dev-tools/db-browser
make
./build/bin/db-browser
```

Requires: GTK+3, libsqlite3, gcc.
Default DB pointed at `page-builder/xcm-editor.db`.

## agent-flow

```bash
php -S localhost:9090 -t dev-tools/agent-flow
```

## Report Generator

```bash
python dev-tools/zyx_planning_and_visuals/make_report.py
```

Runs db-browser smoke analysis and Python audit and prints a project health summary.

## code-review Scripts

| Script | What it does |
|--------|-------------|
| `lines_count.py` | Flag files over a line-count threshold (default 1000) |
| `security_ck.py` | Scan for hardcoded secrets and API keys |
| `py_audit.py` | Python code quality concerns |
| `god_funcs.py` | Flag god-functions (too-large single functions) |
| `search.py` | Project-wide exact / regex / fuzzy search |

```bash
python dev-tools/code-review/lines_count.py /path/to/dir
python dev-tools/code-review/security_ck.py /path/to/dir --no-color
python dev-tools/code-review/search.py "query" --regex
```
