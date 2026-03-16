# page-builder

JSON-driven page composition tool, admin panel, and Go authentication server.

## What This Is For

This page builder is part of a rapid-start workspace meant to turn ideas into working apps quickly.
It is designed as both a practical builder and a source of inspiration for new projects, with features that can be extended as the repo grows.

The bundled auth/database pieces are intended for local dev preview and starter workflows.
Production secrets and sensitive assets should stay outside the repo root and be linked in locally when needed.

## Contents

| Path | Description |
|------|-------------|
| `index.php` | Page index -- list all pages with build status |
| `composer.php` | 3-panel page composer (library / canvas / JSON panels) |
| `build.php` | Reads page.json manifest and renders `index.html` |
| `create-page.php` | Scaffold a new page directory in one step |
| `section-api.php` | Section CRUD API |
| `sections/` | Reusable section templates (headers, footers, sections, panels) |
| `pages/` | Per-page directories, each with `page.json` and `overrides.json` |
| `pb_admin/` | PHP admin panel backed by Go auth API |
| `xcm_auth/` | Go authentication server (port 9100) |
| `bindings/typescript/` | Linked TypeScript render/runtime bindings |
| `public_html/breadcrumb-manager/` | Linked breadcrumb manager runtime assets |
| `xcm-editor.db` | Shared project editor SQLite database |

## How It Works

1. Pages live in `pages/{page-name}/` with a `page.json` manifest
2. `build.php` reads the manifest, merges `overrides.json`, renders `index.html`
3. The composer at `/page-builder/composer.php?page={name}` edits sections live

## Starting the Stack

```bash
# Go auth server (required for admin panel)
bash page-builder/pb_admin/start-auth.sh

# Optional Agent Flow UI used by chatbot/page-builder integrations
php -S 127.0.0.1:9090 -t dev-tools/agent-flow

# Admin panel is served by nginx at https://localhost:8443/page-builder/pb_admin/
# Page builder at https://localhost:8443/page-builder/
```

## Admin Panel (pb_admin)

| Tool | File | Description |
|------|------|-------------|
| Dashboard | `dashboard.php` | Links to all admin tools |
| Auth Status | `tools/01-auth-status.php` | Go server status, active sessions |
| Sessions | `tools/02-sessions.php` | List and invalidate sessions |
| Audit Log | `tools/03-audit-log.php` | Login attempts and token events |
| User Management | `tools/04-users.php` | Create users, set roles |
| Page Builder | `tools/05-page-builder.php` | Page builder admin link plus current script/breadcrumb settings |
| DB Browser | `tools/06-db-browser.php` | Launch the SQLite db-browser |
| Accounting | `tools/07-accounting.php` | Accounting/admin entry point |
| Agent Flow | `tools/08-agent-flow.php` | Quick links to the Agent Flow UI and chatbot API |

`api_proxy.php` proxies PHP requests to the Go auth API on port 9100.

## Go Auth Server (xcm_auth)

Handles login, session validation, logout, and user management.

```
xcm_auth/
  xcm_auth_dev.db      SQLite user + session store
  dev-credentials.json Plaintext dev password (gitignored)
```

Setup (first run):

```bash
bash page-builder/pb_admin/start-auth.sh
# Then visit https://localhost:8443/page-builder/pb_admin/setup.php
```

## Section Schema

All section JSON files follow the schema in
`my_project/vscode-bridge/context/section-schema.md`.

Drop a new `.json` file into `sections/{type}/` and it appears in the library
with no registration step.

## External Language + Breadcrumb Integrations

The page builder can link external runtime artifacts directly into this folder
using Windows junctions/symlinks:

- `page-builder/bindings/typescript` -> `C:\Users\criss\Desktop\render_eng\bindings\typescript`
- `page-builder/public_html/breadcrumb-manager` -> `C:\Users\criss\Desktop\bc_mgr\dist`

Project settings now include:

- `builder_script_language` (`javascript` or `typescript`)
- `supported_script_languages` (array of enabled script languages)
- `breadcrumb_manager_enabled` (`true` or `false`)
- `breadcrumb_manager_package` (`bc_mgr_wasm_with_storage` or `bc_mgr_wasm_dropin`)

These settings are managed through `project.json`, `project-config.php`, and the visual editor UI in `index.php`.
At build time they are passed into the generated page shell so runtime integrations can read the active script mode and breadcrumb package.

## Current Builder Capabilities

- Visual editor/project settings support both JavaScript and TypeScript build modes.
- Built pages receive project-level runtime metadata from `build.php`.
- Breadcrumb manager assets can be injected into the generated output when enabled.
- Admin tooling includes quick access to Agent Flow for chatbot and flow-based integrations.

## Agent Flow Integration

Agent Flow is exposed in the admin panel and can be started separately on port `9090`.
This is intended for chatbot sections and related flow-driven builder integrations rather than the core page rendering path itself.

## xcm-editor.db

Shared project-level SQLite database scanned by the admin db-browser tool.
Path: `page-builder/xcm-editor.db`
