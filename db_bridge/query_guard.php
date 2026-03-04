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

    // ---- Accounting actions (read) ----
    'accounting.accounts_list'  => 'db_action_accounting_accounts_list',
    'accounting.journal_list'   => 'db_action_accounting_journal_list',
    'accounting.journal_get'    => 'db_action_accounting_journal_get',
    'accounting.period_list'    => 'db_action_accounting_period_list',
    'accounting.trial_balance'  => 'db_action_accounting_trial_balance',

    // ---- Accounting actions (write) ----
    'accounting.account_create'  => 'db_action_accounting_account_create',
    'accounting.journal_create'  => 'db_action_accounting_journal_create',
    'accounting.journal_post'    => 'db_action_accounting_journal_post',
];

// Actions that require admin-level auth (checked by the router, not here)
const DB_ADMIN_ACTIONS = [
    'admin.db_list',
    'admin.table_list',
    'admin.table_read',
    'admin.table_count',
    // All accounting actions are admin-gated
    'accounting.accounts_list',
    'accounting.journal_list',
    'accounting.journal_get',
    'accounting.period_list',
    'accounting.trial_balance',
    'accounting.account_create',
    'accounting.journal_create',
    'accounting.journal_post',
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
// Accounting handlers
// ---------------------------------------------------------------------------

function db_action_accounting_accounts_list(PDO $pdo, array $p): array
{
    $allowed_types = ['ASSET', 'LIABILITY', 'EQUITY', 'REVENUE', 'EXPENSE'];
    $type_filter   = strtoupper(trim($p['type'] ?? ''));

    if ($type_filter !== '' && !in_array($type_filter, $allowed_types, true)) {
        throw new InvalidArgumentException('Invalid account type filter.');
    }

    if ($type_filter !== '') {
        $stmt = $pdo->prepare(
            'SELECT id, account_code, account_name, account_type, normal_balance,
                    parent_account_id, is_active, description
               FROM accounts
              WHERE account_type = :type AND is_active = 1
              ORDER BY account_code'
        );
        $stmt->bindValue(':type', $type_filter);
    } else {
        $stmt = $pdo->query(
            'SELECT id, account_code, account_name, account_type, normal_balance,
                    parent_account_id, is_active, description
               FROM accounts
              ORDER BY account_code'
        );
    }
    $stmt->execute();
    return $stmt->fetchAll();
}

function db_action_accounting_journal_list(PDO $pdo, array $p): array
{
    $limit  = min((int)($p['limit']  ?? 50), 200);
    $offset = max((int)($p['offset'] ?? 0), 0);

    $stmt = $pdo->prepare(
        'SELECT je.id, je.entry_number, je.entry_date, je.description,
                je.reference, je.posted, je.created_by, je.created_at,
                COALESCE(SUM(jl.debit_amount), 0) AS total_debits
           FROM journal_entries je
      LEFT JOIN journal_lines jl ON jl.journal_entry_id = je.id
          GROUP BY je.id
          ORDER BY je.entry_date DESC, je.id DESC
          LIMIT :limit OFFSET :offset'
    );
    $stmt->bindValue(':limit',  $limit,  PDO::PARAM_INT);
    $stmt->bindValue(':offset', $offset, PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchAll();
}

function db_action_accounting_journal_get(PDO $pdo, array $p): array
{
    $id = (int)($p['id'] ?? 0);
    if ($id <= 0) {
        throw new InvalidArgumentException('Invalid journal entry id.');
    }

    $stmt = $pdo->prepare(
        'SELECT id, entry_number, entry_date, description, reference,
                posted, created_by, created_at, posted_at
           FROM journal_entries WHERE id = :id LIMIT 1'
    );
    $stmt->bindValue(':id', $id, PDO::PARAM_INT);
    $stmt->execute();
    $entry = $stmt->fetch();
    if (!$entry) {
        return [];
    }

    $lstmt = $pdo->prepare(
        'SELECT jl.id, jl.account_id, a.account_code, a.account_name,
                jl.debit_amount, jl.credit_amount, jl.memo
           FROM journal_lines jl
           JOIN accounts a ON a.id = jl.account_id
          WHERE jl.journal_entry_id = :id
          ORDER BY jl.id'
    );
    $lstmt->bindValue(':id', $id, PDO::PARAM_INT);
    $lstmt->execute();
    $entry['lines'] = $lstmt->fetchAll();

    return $entry;
}

function db_action_accounting_period_list(PDO $pdo, array $p): array
{
    $stmt = $pdo->query(
        'SELECT id, period_name, start_date, end_date, is_closed, closed_at
           FROM fiscal_periods
          ORDER BY start_date DESC'
    );
    return $stmt->fetchAll();
}

function db_action_accounting_trial_balance(PDO $pdo, array $p): array
{
    // Return one row per account with summed debits and credits across all
    // posted journal entries, plus a running balance respecting normal_balance.
    $stmt = $pdo->query(
        'SELECT a.id, a.account_code, a.account_name, a.account_type,
                a.normal_balance,
                COALESCE(SUM(jl.debit_amount),  0) AS total_debits,
                COALESCE(SUM(jl.credit_amount), 0) AS total_credits,
                CASE a.normal_balance
                    WHEN \'DEBIT\'  THEN COALESCE(SUM(jl.debit_amount), 0)
                                       - COALESCE(SUM(jl.credit_amount), 0)
                    WHEN \'CREDIT\' THEN COALESCE(SUM(jl.credit_amount), 0)
                                       - COALESCE(SUM(jl.debit_amount), 0)
                    ELSE 0
                END AS balance
           FROM accounts a
      LEFT JOIN journal_lines jl ON jl.account_id = a.id
      LEFT JOIN journal_entries je ON je.id = jl.journal_entry_id AND je.posted = 1
          WHERE a.is_active = 1
          GROUP BY a.id
          ORDER BY a.account_code'
    );
    return $stmt->fetchAll();
}

function db_action_accounting_account_create(PDO $pdo, array $p): array
{
    $allowed_types    = ['ASSET', 'LIABILITY', 'EQUITY', 'REVENUE', 'EXPENSE'];
    $allowed_balances = ['DEBIT', 'CREDIT'];

    $code   = db_guard_text($p['account_code'] ?? '', 20);
    $name   = db_guard_text($p['account_name'] ?? '', 100);
    $type   = db_guard_enum(strtoupper(trim($p['account_type'] ?? '')), $allowed_types);
    $bal    = db_guard_enum(strtoupper(trim($p['normal_balance'] ?? '')), $allowed_balances);
    $desc   = db_guard_text($p['description'] ?? '', 500);
    $parent = isset($p['parent_account_id']) ? (int)$p['parent_account_id'] : null;

    if ($code === '' || $name === '') {
        throw new InvalidArgumentException('account_code and account_name are required.');
    }

    $stmt = $pdo->prepare(
        'INSERT INTO accounts (account_code, account_name, account_type, normal_balance,
                               parent_account_id, description, is_active)
              VALUES (:code, :name, :type, :bal, :parent, :desc, 1)'
    );
    $stmt->bindValue(':code',   $code);
    $stmt->bindValue(':name',   $name);
    $stmt->bindValue(':type',   $type);
    $stmt->bindValue(':bal',    $bal);
    $stmt->bindValue(':parent', $parent, $parent === null ? PDO::PARAM_NULL : PDO::PARAM_INT);
    $stmt->bindValue(':desc',   $desc);
    $stmt->execute();

    return ['id' => (int)$pdo->lastInsertId(), 'account_code' => $code];
}

function db_action_accounting_journal_create(PDO $pdo, array $p): array
{
    $entry_date  = db_guard_text($p['entry_date']  ?? '', 10);
    $description = db_guard_text($p['description'] ?? '', 500);
    $reference   = db_guard_text($p['reference']   ?? '', 100);
    $created_by  = db_guard_text($p['created_by']  ?? '', 100);
    $lines       = $p['lines'] ?? [];

    if ($entry_date === '' || $description === '') {
        throw new InvalidArgumentException('entry_date and description are required.');
    }
    if (!preg_match('/^\d{4}-\d{2}-\d{2}$/', $entry_date)) {
        throw new InvalidArgumentException('entry_date must be YYYY-MM-DD.');
    }
    if (!is_array($lines) || count($lines) < 2) {
        throw new InvalidArgumentException('A journal entry requires at least two lines.');
    }

    // Validate double-entry: total debits must equal total credits
    $total_debit  = 0.0;
    $total_credit = 0.0;
    $clean_lines  = [];

    foreach ($lines as $line) {
        $account_id    = (int)($line['account_id']    ?? 0);
        $debit_amount  = round((float)($line['debit_amount']  ?? 0), 2);
        $credit_amount = round((float)($line['credit_amount'] ?? 0), 2);
        $memo          = db_guard_text($line['memo'] ?? '', 255);

        if ($account_id <= 0) {
            throw new InvalidArgumentException('Each line must have a valid account_id.');
        }
        if (($debit_amount > 0 && $credit_amount > 0) || ($debit_amount === 0.0 && $credit_amount === 0.0)) {
            throw new InvalidArgumentException('Each line must have either a debit or a credit amount, not both or neither.');
        }

        $total_debit  += $debit_amount;
        $total_credit += $credit_amount;
        $clean_lines[] = compact('account_id', 'debit_amount', 'credit_amount', 'memo');
    }

    if (abs($total_debit - $total_credit) > 0.005) {
        throw new InvalidArgumentException(
            sprintf('Entry is not balanced: debits %.2f != credits %.2f', $total_debit, $total_credit)
        );
    }

    // Auto-generate an entry number (JE-YYYYMMDD-NNNN)
    $count_row    = $pdo->query('SELECT COUNT(*) FROM journal_entries')->fetchColumn();
    $entry_number = 'JE-' . date('Ymd') . '-' . str_pad((int)$count_row + 1, 4, '0', STR_PAD_LEFT);

    $pdo->beginTransaction();
    try {
        $stmt = $pdo->prepare(
            'INSERT INTO journal_entries (entry_number, entry_date, description,
                                         reference, posted, created_by)
                  VALUES (:num, :date, :desc, :ref, 0, :by)'
        );
        $stmt->bindValue(':num',  $entry_number);
        $stmt->bindValue(':date', $entry_date);
        $stmt->bindValue(':desc', $description);
        $stmt->bindValue(':ref',  $reference);
        $stmt->bindValue(':by',   $created_by);
        $stmt->execute();

        $entry_id = (int)$pdo->lastInsertId();

        $lstmt = $pdo->prepare(
            'INSERT INTO journal_lines (journal_entry_id, account_id, debit_amount,
                                        credit_amount, memo)
                  VALUES (:je, :acct, :dr, :cr, :memo)'
        );
        foreach ($clean_lines as $l) {
            $lstmt->bindValue(':je',   $entry_id,          PDO::PARAM_INT);
            $lstmt->bindValue(':acct', $l['account_id'],   PDO::PARAM_INT);
            $lstmt->bindValue(':dr',   $l['debit_amount']);
            $lstmt->bindValue(':cr',   $l['credit_amount']);
            $lstmt->bindValue(':memo', $l['memo']);
            $lstmt->execute();
        }

        $pdo->commit();
    } catch (Throwable $e) {
        $pdo->rollBack();
        throw $e;
    }

    return ['id' => $entry_id, 'entry_number' => $entry_number];
}

function db_action_accounting_journal_post(PDO $pdo, array $p): array
{
    $id = (int)($p['id'] ?? 0);
    if ($id <= 0) {
        throw new InvalidArgumentException('Invalid journal entry id.');
    }

    // Verify the entry is not already posted
    $chk = $pdo->prepare('SELECT posted FROM journal_entries WHERE id = :id');
    $chk->bindValue(':id', $id, PDO::PARAM_INT);
    $chk->execute();
    $row = $chk->fetch();
    if (!$row) {
        throw new RuntimeException('Journal entry not found.');
    }
    if ((int)$row['posted'] === 1) {
        throw new RuntimeException('Entry is already posted.');
    }

    $stmt = $pdo->prepare(
        'UPDATE journal_entries
            SET posted = 1, posted_at = datetime(\'now\')
          WHERE id = :id'
    );
    $stmt->bindValue(':id', $id, PDO::PARAM_INT);
    $stmt->execute();

    return ['id' => $id, 'posted' => true];
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
