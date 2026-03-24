# Dev Tools Workspace

**Author:** Crissy Deutsch
**Company:** XcaliburMoon Web Development
**Website:** https://xcaliburmoon.net/
**License:** MIT

Monorepo for the style tool, page builder, auth/runtime services, native browser
targets, and internal automation utilities.

## Quick Start

```bash
# Start style tool
cd my_project
php -S 127.0.0.1:9879 index.php

# Start full local stack
bash server/start.sh
bash page-builder/pb_admin/start-auth.sh

# Generate docs/reports and push
bash push.sh
powershell -ExecutionPolicy Bypass -File .\push-win.ps1
```

## Project Notes

This repository is meant to be an inspiration and rapid-start workspace for building apps quickly.
It combines experiments, starter flows, and reusable tooling in one growing repo so ideas can move into working prototypes with minimal setup.

Local auth/database values that appear in some parts of the repo are intended for dev preview and starter workflows only, not for production secrets.
Sensitive material is expected to stay outside the repo root and be linked in locally when needed.

## Directory Map

| Folder | Description | README |
|--------|-------------|--------|
| `my_project/` | Live HTML/CSS/JS editor with AI agent and VSCode bridge. | [README](my_project/README.md) |
| `page-builder/` | JSON-driven page composer, visual editor, and admin panel. | [README](page-builder/README.md) |
| `dev-tools/` | Utilities, scanners, automation, and planning/reporting tools. | [README](dev-tools/README.md) |
| `imgui-browser/` | Cross-platform native browser shell (C++ / Dear ImGui). | [README](imgui-browser/README.md) |
| `server/` | Nginx/PHP-FPM startup scripts and local runtime controls. | [README](server/README.md) |
| `db_bridge/` | Shared SQLite bridge API (PHP + JS client). | [README](db_bridge/README.md) |

## Project Structure

```
dev_tools/
  my_project/  -- Live HTML/CSS/JS editor with AI agent and VSCode bridge.
  page-builder/  -- JSON-driven page composer, visual editor, and admin panel.
  dev-tools/  -- Utilities, scanners, automation, and planning/reporting tools.
  imgui-browser/  -- Cross-platform native browser shell (C++ / Dear ImGui).
  server/  -- Nginx/PHP-FPM startup scripts and local runtime controls.
  db_bridge/  -- Shared SQLite bridge API (PHP + JS client).
  make_readme.py
  push.sh
  push-win.ps1
  smoke-tools.json
  launcher.json
```

## Automation

Push scripts run generation steps before staging commits:

1. Regenerate root README via `make_readme.py`.
2. Generate report artifacts via `dev-tools/zyx_planning_and_visuals/make_report.py`.

Manual report run:

```bash
python dev-tools/zyx_planning_and_visuals/make_report.py
```

## License

MIT License. See [LICENSE](LICENSE).

---

*README last generated: 2026-03-24 07:03:31 UTC*
