<?php
/**
 * db_bridge/query_guard.php
 *
 * Allowlist-only action dispatcher.
 * No raw SQL ever reaches this layer from the client.
 * Every permitted action is mapped to a fully parameterized query.
 *
 * Usage (internal, called from api.php or routers):
 *   require_once __DIR__ . '/query_guard.php';
 *   $result = db_dispatch($action, $params, $pdo);
 */

declare(strict_types=1);

require_once __DIR__ . '/connection.php';

// ---------------------------------------------------------------------------
// Allowlist: action => callable that receives (PDO $pdo, array $p): array
// ---------------------------------------------------------------------------
const DB_ACTIONS = [
    // ---- Page-builder actions ----
    'pages.list'    => 'db_action_pages_list',
    'pages.get'     => 'db_action_pages_get',
    'pages.upsert'  => 'db_action_pages_upsert',
    'pages.history' => 'db_action_pages_history',
    'pages.delete'  => 'db_action_pages_delete',

    // ---- Admin read actions ----
    'admin.db_list'      => 'db_action_admin_db_list',
    'admin.table_list'   => 'db_action_admin_table_list',
    'admin.table_read'   => 'db_action_admin_table_read',
    'admin.table_count'  => 'db_action_admin_table_count',
];

// Actions that require admin-level auth (checked by the router, not here)
const DB_ADMIN_ACTIONS = [
    'admin.db_list',
    'admin.table_list',
    'admin.table_read',
    'admin.table_count',
];

/**
 * Dispatch an allowlisted action.
 *
 * @param  string  $action  e.g. "pages.list"
 * @param  array   $params  Untrusted client params (sanitised inside each handler)
 * @param  PDO     $pdo     An open database connection
 * @return array            ['ok' => bool, 'data' => mixed, 'error' => string|null]
 */
function db_dispatch(string $action, array $params, PDO $pdo): array
{
    if (!isset(DB_ACTIONS[$action])) {
        return ['ok' => false, 'error' => 'Unknown action: ' . $action, 'data' => null];
    }

    $handler = DB_ACTIONS[$action];
    try {
        $data = $handler($pdo, $params);
        return ['ok' => true, 'data' => $data, 'error' => null];
    } catch (Throwable $e) {
        error_log('[db_bridge] ' . $action . ': ' . $e->getMessage());
        return ['ok' => false, 'error' => 'Query failed.', 'data' => null];
    }
}

// ---------------------------------------------------------------------------
// Page-builder handlers
// ---------------------------------------------------------------------------

function db_action_pages_list(PDO $pdo, array $p): array
{
    $limit  = min((int)($p['limit']  ?? 100), 500);
    $offset = max((int)($p['offset'] ?? 0),     0);

    $stmt = $pdo->prepare(
        'SELECT id, slug, title, status, updated_at
           FROM pages
          ORDER BY updated_at DESC
          LIMIT :limit OFFSET :offset'
    );
    $stmt->bindValue(':limit',  $limit,  PDO::PARAM_INT);
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchAll();
}

function db_action_pages_get(PDO $pdo, array $p): array
{
    $slug = db_guard_slug($p['slug'] ?? '');

    $stmt = $pdo->prepare(
        'SELECT id, slug, title, status, css_overrides, meta, created_at, updated_at
           FROM pages
          WHERE slug = :slug
          LIMIT 1'
    );
    $stmt->bindValue(':slug', $slug);
    $stmt->execute();
    $row = $stmt->fetch();
    return $row ?: [];
}

function db_action_pages_upsert(PDO $pdo, array $p): array
{
    $slug      = db_guard_slug($p['slug']          ?? '');
    $title     = db_guard_text($p['title']         ?? '', 255);
    $status    = db_guard_enum($p['status']        ?? 'draft', ['draft', 'published', 'archived']);
    $overrides = db_guard_json($p['css_overrides'] ?? '{}');
    $meta      = db_guard_json($p['meta']          ?? '{}');

    $stmt = $pdo->prepare(
        'INSERT INTO pages (slug, title, status, css_overrides, meta, created_at, updated_at)
              VALUES (:slug, :title, :status, :overrides, :meta,
                      strftime(\'%s\', \'now\'), strftime(\'%s\', \'now\'))
              ON CONFLICT(slug) DO UPDATE SET
                  title        = excluded.title,
                  status       = excluded.status,
                  css_overrides = excluded.css_overrides,
                  meta         = excluded.meta,
                  updated_at   = strftime(\'%s\', \'now\')'
    );
    $stmt->bindValue(':slug',      $slug);
    $stmt->bindValue(':title',     $title);
    $stmt->bindValue(':status',    $status);
    $stmt->bindValue(':overrides', $overrides);
    $stmt->bindValue(':meta',      $meta);
    $stmt->execute();

    return ['affected' => $stmt->rowCount(), 'slug' => $slug];
}

function db_action_pages_history(PDO $pdo, array $p): array
{
    $slug  = db_guard_slug($p['slug']    ?? '');
    $limit = min((int)($p['limit'] ?? 20), 100);

    $stmt = $pdo->prepare(
        'SELECT id, slug, css_overrides, saved_at, saved_by
           FROM page_history
          WHERE slug = :slug
          ORDER BY saved_at DESC
          LIMIT :limit'
    );
    $stmt->bindValue(':slug',  $slug);
    $stmt->bindValue(':limit', $limit, PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchAll();
}

function db_action_pages_delete(PDO $pdo, array $p): array
{
    $slug = db_guard_slug($p['slug'] ?? '');
    $stmt = $pdo->prepare('DELETE FROM pages WHERE slug = :slug');
    $stmt->bindValue(':slug', $slug);
    $stmt->execute();
    return ['affected' => $stmt->rowCount()];
}

// ---------------------------------------------------------------------------
// Admin handlers (read-only for safety)
// ---------------------------------------------------------------------------

function db_action_admin_db_list(PDO $pdo, array $p): array
{
    // pdo not used - lists workspace databases
    return db_list_all();
}

function db_action_admin_table_list(PDO $pdo, array $p): array
{
    $stmt = $pdo->query(
        "SELECT name, type FROM sqlite_master
          WHERE type IN ('table','view')
            AND name NOT LIKE 'sqlite_%'
          ORDER BY type, name"
    );
    return $stmt->fetchAll();
}

function db_action_admin_table_read(PDO $pdo, array $p): array
{
    $table  = db_guard_identifier($p['table']  ?? '');
    $limit  = min((int)($p['limit']  ?? 50), 500);
    $offset = max((int)($p['offset'] ?? 0),    0);

    // Verify table exists before constructing the query
    $chk = $pdo->prepare(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?"
    );
    $chk->execute([$table]);
    if (!$chk->fetch()) {
        throw new RuntimeException('Table not found: ' . $table);
    }

    // Table name is now safe to interpolate (validated as existing identifier)
    $stmt = $pdo->prepare(
        "SELECT * FROM \"$table\" LIMIT :limit OFFSET :offset"
    );
    $stmt->bindValue(':limit',  $limit,  PDO::PARAM_INT);
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchAll();
}

function db_action_admin_table_count(PDO $pdo, array $p): array
{
    $table = db_guard_identifier($p['table'] ?? '');

    $chk = $pdo->prepare(
        "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?"
    );
    $chk->execute([$table]);
    if (!$chk->fetch()) {
        throw new RuntimeException('Table not found: ' . $table);
    }

    $row = $pdo->query("SELECT COUNT(*) AS cnt FROM \"$table\"")->fetch();
    return ['table' => $table, 'count' => (int)$row['cnt']];
}

// ---------------------------------------------------------------------------
// Input guards  (private helpers, not part of public API)
// ---------------------------------------------------------------------------

/** Page slug: lowercase alphanumeric, dashes, underscores, max 120 chars */
function db_guard_slug(string $v): string
{
    $v = strtolower(trim($v));
    if (!preg_match('/^[a-z0-9_\-]{1,120}$/', $v)) {
        throw new InvalidArgumentException('Invalid slug.');
    }
    return $v;
}

/** Plain text stripped of tags and truncated */
function db_guard_text(string $v, int $max): string
{
    return substr(strip_tags(trim($v)), 0, $max);
}

/** Enum value constrained to an explicit whitelist */
function db_guard_enum(string $v, array $allowed): string
{
    if (!in_array($v, $allowed, true)) {
        throw new InvalidArgumentException('Invalid enum value: ' . $v);
    }
    return $v;
}

/** Re-encode JSON to strip any injected content */
function db_guard_json(string $v): string
{
    $decoded = json_decode($v, true);
    if ($decoded === null && json_last_error() !== JSON_ERROR_NONE) {
        return '{}';
    }
    return json_encode($decoded, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
}

/** SQLite identifier (table/column name) - strip everything except word chars */
function db_guard_identifier(string $v): string
{
    $v = trim($v);
    if (!preg_match('/^[a-zA-Z_][a-zA-Z0-9_]{0,63}$/', $v)) {
        throw new InvalidArgumentException('Invalid identifier: ' . $v);
    }
    return $v;
}
