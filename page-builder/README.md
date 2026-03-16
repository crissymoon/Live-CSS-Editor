# page-builder

JSON-driven page composition tool, admin panel, and Go authentication server.

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
| `xcm-editor.db` | Shared project editor SQLite database |

## How It Works

1. Pages live in `pages/{page-name}/` with a `page.json` manifest
2. `build.php` reads the manifest, merges `overrides.json`, renders `index.html`
3. The composer at `/page-builder/composer.php?page={name}` edits sections live

## Starting the Stack

```bash
# Go auth server (required for admin panel)
bash page-builder/pb_admin/start-auth.sh

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
| Page Builder | `tools/05-page-builder.php` | Page builder admin link |
| DB Browser | `tools/06-db-browser.php` | Launch the SQLite db-browser |

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

## xcm-editor.db

Shared project-level SQLite database scanned by the admin db-browser tool.
Path: `page-builder/xcm-editor.db`
