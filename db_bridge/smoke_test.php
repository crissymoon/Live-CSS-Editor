#!/usr/bin/env php
<?php
/**
 * db_bridge/smoke_test.php
 *
 * CLI smoke test for the db_bridge layer.
 * Tests connection, query guards, dispatch, and auth helpers end-to-end
 * without requiring a running HTTP server.
 *
 * Usage:
 *   php db_bridge/smoke_test.php
 *
 * Exit code 0 = all tests passed, 1 = at least one failure.
 */

declare(strict_types=1);

define('SMOKE_ROOT', dirname(__DIR__));

$pass    = 0;
$fail    = 0;
$results = [];

// ---- Test runner helpers ---------------------------------------------------

function ok(string $name, bool $cond, string $detail = ''): void
{
    global $pass, $fail, $results;
    if ($cond) {
        $pass++;
        $results[] = "[PASS] $name";
    } else {
        $fail++;
        $results[] = "[FAIL] $name" . ($detail ? " -- $detail" : '');
    }
}

function section(string $name): void
{
    echo "\n-- $name\n";
}

// ---- Load modules ----------------------------------------------------------

require_once __DIR__ . '/connection.php';
require_once __DIR__ . '/query_guard.php';

// ---- 1. connection.php -----------------------------------------------------

section('connection.php');

$db_path = SMOKE_ROOT . '/dev-tools/db-browser/databases/pages.db';

ok('pages.db file exists', file_exists($db_path), $db_path);

try {
    $pdo = db_connect($db_path);
    ok('db_connect() returns PDO', $pdo instanceof PDO);
    $mode = $pdo->query('PRAGMA journal_mode')->fetchColumn();
    ok('journal_mode is WAL', $mode === 'wal', "got: $mode");
    $fk = $pdo->query('PRAGMA foreign_keys')->fetchColumn();
    ok('foreign_keys ON', (int)$fk === 1, "got: $fk");
    $sync = $pdo->query('PRAGMA synchronous')->fetchColumn();
    ok('synchronous=NORMAL (1)', (int)$sync === 1, "got: $sync");
} catch (Throwable $e) {
    ok('db_connect() no exception', false, $e->getMessage());
    $pdo = null;
}

// db_resolve_path guards
section('connection.php -- db_resolve_path()');

$threw = false;
try { db_resolve_path('../../etc/passwd'); } catch (InvalidArgumentException) { $threw = true; }
ok('rejects ../traversal', $threw);

$threw = false;
try { db_resolve_path('name with spaces!'); } catch (InvalidArgumentException) { $threw = true; }
ok('rejects invalid chars', $threw);

$resolved = db_resolve_path('dev-tools/db-browser/databases/pages');
ok('resolves valid path to .db',
    str_ends_with($resolved, '.db'),
    $resolved
);

// ---- 2. query_guard.php input guards  -------------------------------------

section('query_guard.php -- input guards');

// db_guard_slug
$threw = false;
try { db_guard_slug(''); } catch (InvalidArgumentException) { $threw = true; }
ok('guard_slug rejects empty', $threw);

$threw = false;
try { db_guard_slug('../../bad'); } catch (InvalidArgumentException) { $threw = true; }
ok('guard_slug rejects path chars', $threw);

ok('guard_slug accepts valid slug', db_guard_slug('my-page_01') === 'my-page_01');

// db_guard_enum
$threw = false;
try { db_guard_enum('hacked', ['draft', 'published']); } catch (InvalidArgumentException) { $threw = true; }
ok('guard_enum rejects unknown value', $threw);
ok('guard_enum accepts valid value', db_guard_enum('published', ['draft', 'published']) === 'published');

// db_guard_json
ok('guard_json re-encodes valid JSON', db_guard_json('{"a":1}') === '{"a":1}');
ok('guard_json returns {} for broken JSON', db_guard_json('{bad') === '{}');
// Test XSS injection in JSON value is normalised
$xss_in  = '{"title":"<script>alert(1)</script>"}';
$xss_out = db_guard_json($xss_in);
ok('guard_json re-encodes (does not strip) JSON content', json_decode($xss_out, true) !== null);

// db_guard_identifier
$threw = false;
try { db_guard_identifier('users; DROP TABLE pages'); } catch (InvalidArgumentException) { $threw = true; }
ok('guard_identifier rejects SQL injection', $threw);
ok('guard_identifier accepts valid name', db_guard_identifier('page_history') === 'page_history');

// ---- 3. db_dispatch() -- pages.* actions  ----------------------------------

if ($pdo !== null) {
    section('db_dispatch() -- pages.*');

    // pages.list on empty db
    $r = db_dispatch('pages.list', ['limit' => 5], $pdo);
    ok('pages.list ok=true', $r['ok'] === true, json_encode($r['error'] ?? ''));
    ok('pages.list data is array', is_array($r['data']), gettype($r['data']));

    // pages.upsert
    $r = db_dispatch('pages.upsert', [
        'slug'         => 'smoke-test-page',
        'title'        => 'Smoke Test Page',
        'status'       => 'draft',
        'css_overrides'=> '{"color":"red"}',
        'meta'         => '{}',
    ], $pdo);
    ok('pages.upsert ok=true', $r['ok'] === true, json_encode($r['error'] ?? ''));
    ok('pages.upsert affected >= 1', ($r['data']['affected'] ?? 0) >= 1);

    // pages.get - retrieve what we just inserted
    $r = db_dispatch('pages.get', ['slug' => 'smoke-test-page'], $pdo);
    ok('pages.get ok=true', $r['ok'] === true);
    ok('pages.get returns slug', ($r['data']['slug'] ?? '') === 'smoke-test-page');
    ok('pages.get returns css_overrides', isset($r['data']['css_overrides']));

    // pages.list now has at least 1 row
    $r = db_dispatch('pages.list', ['limit' => 10], $pdo);
    ok('pages.list finds inserted row', count($r['data'] ?? []) >= 1);

    // pages.history
    $r = db_dispatch('pages.history', ['slug' => 'smoke-test-page'], $pdo);
    ok('pages.history ok=true', $r['ok'] === true);

    // pages.delete
    $r = db_dispatch('pages.delete', ['slug' => 'smoke-test-page'], $pdo);
    ok('pages.delete ok=true', $r['ok'] === true);
    ok('pages.delete affected=1', ($r['data']['affected'] ?? 0) === 1);

    // Confirm deleted
    $r = db_dispatch('pages.get', ['slug' => 'smoke-test-page'], $pdo);
    ok('pages.get returns empty after delete', $r['ok'] === true && empty($r['data']));

    section('db_dispatch() -- admin.*');

    // admin.table_list
    $r = db_dispatch('admin.table_list', [], $pdo);
    ok('admin.table_list ok=true', $r['ok'] === true);
    $table_names = array_column($r['data'] ?? [], 'name');
    ok('admin.table_list includes pages table', in_array('pages', $table_names, true));

    // admin.table_count
    $r = db_dispatch('admin.table_count', ['table' => 'pages'], $pdo);
    ok('admin.table_count ok=true', $r['ok'] === true);
    ok('admin.table_count has count key', isset($r['data']['count']));

    // admin.table_read
    $r = db_dispatch('admin.table_read', ['table' => 'pages', 'limit' => 5], $pdo);
    ok('admin.table_read ok=true', $r['ok'] === true);

    // admin.table_read with injected table name
    $r = db_dispatch('admin.table_read', ['table' => "pages; SELECT 1--"], $pdo);
    ok('admin.table_read rejects bad table', $r['ok'] === false);

    // admin.db_list
    $r = db_dispatch('admin.db_list', [], $pdo);
    ok('admin.db_list ok=true', $r['ok'] === true);
    ok('admin.db_list returns array', is_array($r['data']));
    ok('admin.db_list finds pages.db', count(array_filter(
        $r['data'] ?? [],
        fn($d) => str_ends_with($d['path'] ?? '', 'pages.db')
    )) > 0);
}

// ---- 4. action allowlist ---------------------------------------------------

section('db_dispatch() -- allowlist');

$dummy_pdo = new PDO('sqlite::memory:');
$r = db_dispatch('sql.injection', [], $dummy_pdo);
ok('unknown action returns ok=false', $r['ok'] === false);
ok('unknown action error mentions action', str_contains($r['error'] ?? '', 'Unknown action'));

// ---- 5. db_list_all() sanity -----------------------------------------------

section('db_list_all()');

$dbs = db_list_all(SMOKE_ROOT);
ok('db_list_all() returns array', is_array($dbs));
ok('db_list_all() finds at least 1 db', count($dbs) >= 1);
$paths = array_column($dbs, 'path');
$found_pages = array_filter($paths, fn($p) => str_ends_with($p, 'pages.db'));
ok('db_list_all() finds pages.db', count($found_pages) > 0);

// ---- Results ---------------------------------------------------------------

echo "\n" . str_repeat('=', 50) . "\n";
foreach ($results as $line) {
    echo $line . "\n";
}
echo str_repeat('=', 50) . "\n";
$total = $pass + $fail;
echo "RESULT: $pass/$total passed";
if ($fail > 0) {
    echo "  ($fail FAILED)";
}
echo "\n";

exit($fail > 0 ? 1 : 0);
