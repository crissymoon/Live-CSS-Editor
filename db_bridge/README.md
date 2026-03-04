# db_bridge

Secure PHP + JS layer connecting **page-builder** and **pb_admin** to the SQLite
database system managed by the C db-browser tool.

---

## Files

| File | Purpose |
|---|---|
| `connection.php` | PDO factory — WAL/cache/mmap settings matching the C layer |
| `query_guard.php` | Allowlist dispatcher — no raw SQL from clients, ever |
| `api.php` | Central HTTP endpoint (POST JSON) |
| `db-client.js` | Browser ES module — `DbClient` class + factory helpers |
| `schema/pages.sql` | Pages database schema (WAL, history trigger) |
| `setup.sh` | One-shot database initialiser |

---

## Authentication

| Caller | Auth method |
|---|---|
| **pb_admin** front-end | `xcm_auth` session cookie (set by pb_admin/auth.php) |
| **page-builder** front-end | `Bearer <stage-token>` from `.stage-token` file OR xcm_auth session |
| Admin actions (`admin.*`) | xcm_auth session only — stage-token is NOT accepted |

---

## Actions

### Page-builder actions (token or session)

| Action | Params | Returns |
|---|---|---|
| `pages.list` | `limit`, `offset` | Array of page rows |
| `pages.get` | `slug` | Single page row |
| `pages.upsert` | `slug`, `title`, `status`, `css_overrides` (JSON), `meta` (JSON) | `{affected, slug}` |
| `pages.history` | `slug`, `limit` | Array of history rows |
| `pages.delete` | `slug` | `{affected}` |

### Admin actions (session only)

| Action | Params | Returns |
|---|---|---|
| `admin.db_list` | — | All workspace `.db` files |
| `admin.table_list` | — | Tables in selected db |
| `admin.table_read` | `table`, `limit`, `offset` | Rows |
| `admin.table_count` | `table` | `{table, count}` |

---

## Usage examples

### page-builder (vanilla JS)

```html
<meta name="stage-token" content="<?php echo htmlspecialchars(trim(file_get_contents(__DIR__.'/.stage-token'))); ?>">
<script type="module">
import { getPageBuilderClient } from '/db_bridge/db-client.js';
const db = getPageBuilderClient();
const { ok, data } = await db.listPages(20);
if (ok) console.log(data);
</script>
```

### pb_admin (vanilla JS)

```js
import { getAdminClient } from '/db_bridge/db-client.js';
const db = getAdminClient();
const { ok, data } = await db.listTables();
```

### PHP internal (from any PHP file)

```php
require_once __DIR__ . '/../db_bridge/connection.php';
require_once __DIR__ . '/../db_bridge/query_guard.php';

$pdo    = db_connect(db_resolve_path('dev-tools/db-browser/databases/pages'));
$result = db_dispatch('pages.list', ['limit' => 10], $pdo);
```

---

## Database

The pages database lives at:
```
dev-tools/db-browser/databases/pages.db
```

Re-initialise (preserves existing data via `IF NOT EXISTS`):
```bash
bash db_bridge/setup.sh
```

Force rebuild (destructive):
```bash
bash db_bridge/setup.sh --force
```

---

## Security notes

- All client input passes through typed guard functions in `query_guard.php`
  before ever touching a query.
- Table/column identifiers are validated against `sqlite_master` before
  interpolation — no dynamic SQL from untrusted strings.
- JSON fields are decoded and re-encoded to strip any injected content.
- Stage-token comparison uses `hash_equals()` to prevent timing attacks.
- The `db_resolve_path()` function rejects `..` traversal and enforces that all
  resolved paths remain under the workspace root.
