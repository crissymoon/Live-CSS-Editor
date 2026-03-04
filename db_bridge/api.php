<?php
/**
 * db_bridge/api.php
 *
 * Central HTTP endpoint for all database actions.
 * Auth is ALWAYS checked before dispatch:
 *   - admin.* actions require a valid xcm_auth session (Bearer token)
 *   - pages.* actions accept either xcm_auth session OR a stage-token
 *
 * POST /db_bridge/api.php
 * Content-Type: application/json
 * {
 *   "action": "pages.list",
 *   "db":     "page-builder/pages",   // optional: defaults to pages db
 *   "params": { ... }
 * }
 *
 * Response:
 * { "ok": true,  "data": [...] }
 * { "ok": false, "error": "message" }
 */

declare(strict_types=1);

// Allow the browser to hit this from pb_admin and page-builder origins.
// Adjust in production to specific allowed origins.
header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');

// Auth helpers from pb_admin (provides is_logged_in / access_token)
$pb_admin_auth = dirname(__DIR__) . '/pb_admin/auth.php';
$pb_admin_conf = dirname(__DIR__) . '/pb_admin/config.php';

$has_session_auth = file_exists($pb_admin_conf) && file_exists($pb_admin_auth);
if ($has_session_auth) {
    require_once $pb_admin_conf;
    require_once $pb_admin_auth;
}

require_once __DIR__ . '/query_guard.php'; // also pulls in connection.php

// ---- Helpers ----------------------------------------------------------------

function api_json(array $payload, int $status = 200): never
{
    http_response_code($status);
    echo json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function api_error(string $msg, int $status = 400): never
{
    api_json(['ok' => false, 'error' => $msg, 'data' => null], $status);
}

// ---- Stage token auth (page-builder dev/staging) ----------------------------

function is_stage_token_valid(): bool
{
    // Accept token from Authorization header or POST body field
    $token_file = dirname(__DIR__) . '/page-builder/.stage-token';
    if (!file_exists($token_file)) {
        return false;
    }
    $expected = trim(file_get_contents($token_file));
    if (empty($expected)) {
        return false;
    }

    // Bearer header
    $header = $_SERVER['HTTP_AUTHORIZATION'] ?? $_SERVER['HTTP_X_STAGE_TOKEN'] ?? '';
    if (preg_match('/^Bearer\s+(.+)$/i', $header, $m) && hash_equals($expected, $m[1])) {
        return true;
    }

    return false;
}

// ---- Request parsing --------------------------------------------------------

// Allow internal include from pb_admin/api_proxy.php (avoids re-reading stdin)
if (isset($GLOBALS['_DB_BRIDGE_REQUEST']) && is_array($GLOBALS['_DB_BRIDGE_REQUEST'])) {
    $req = $GLOBALS['_DB_BRIDGE_REQUEST'];
    unset($GLOBALS['_DB_BRIDGE_REQUEST']);
} else {
    if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
        http_response_code(204);
        exit;
    }

    if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
        api_error('Method not allowed.', 405);
    }

    $body = file_get_contents('php://input');
    $req  = json_decode($body, true);
}

if (!is_array($req)) {
    api_error('Invalid JSON body.');
}

$action = trim($req['action'] ?? '');
$params = $req['params'] ?? [];
$db_key = trim($req['db']   ?? '');

if ($action === '') {
    api_error('Missing action.');
}

if (!is_array($params)) {
    api_error('Params must be an object.');
}

// ---- Auth gate --------------------------------------------------------------

$is_admin_action = in_array($action, DB_ADMIN_ACTIONS, true);

if ($is_admin_action) {
    // Admin actions: must have a valid xcm_auth session
    if (!$has_session_auth || !is_logged_in()) {
        api_error('Authentication required.', 401);
    }
} else {
    // Page-builder actions: accept xcm_auth session OR valid stage token
    $authed = ($has_session_auth && is_logged_in()) || is_stage_token_valid();
    if (!$authed) {
        api_error('Authentication required.', 401);
    }
}

// ---- Database selection -----------------------------------------------------

// Default database path based on action namespace
if ($db_key === '') {
    if (str_starts_with($action, 'admin.')) {
        // admin.db_list doesn't need a pdo; others need a db specified
        if ($action !== 'admin.db_list') {
            api_error('Missing db parameter for admin action.');
        }
    } else {
        $db_key = 'dev-tools/db-browser/databases/pages';
    }
}

// Resolve and connect
$pdo = null;
if ($db_key !== '' && $action !== 'admin.db_list') {
    try {
        $db_path = db_resolve_path($db_key);
        $pdo     = db_connect($db_path, $is_admin_action);
    } catch (Throwable $e) {
        api_error('Database unavailable: ' . $e->getMessage(), 503);
    }
} elseif ($action !== 'admin.db_list') {
    // Fallback: create in-memory for actions that need a blank slate
    $pdo = new PDO('sqlite::memory:');
}

// ---- Dispatch ---------------------------------------------------------------

if ($pdo === null && $action !== 'admin.db_list') {
    api_error('No database connection.', 503);
}

$result = db_dispatch($action, $params, $pdo ?? new PDO('sqlite::memory:'));
api_json($result, $result['ok'] ? 200 : 500);
