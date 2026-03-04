<?php
/**
 * page-builder/db_api.php
 *
 * Database endpoint for the page-builder front-end.
 * Auth: stage-token (.stage-token file) for dev/staging,
 *       OR a valid xcm_auth session when pb_admin auth is present.
 *
 * POST /page-builder/db_api.php
 * Authorization: Bearer <stage-token>
 * Content-Type: application/json
 * { "action": "pages.list", "params": { ... } }
 *
 * The actual dispatch is handled by db_bridge/api.php (included internally).
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');
// Allow calls from the same host on any port (dev convenience)
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Authorization, Content-Type, X-Stage-Token');
header('Access-Control-Allow-Methods: POST, OPTIONS');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    http_response_code(405);
    echo json_encode(['ok' => false, 'error' => 'Method not allowed.']);
    exit;
}

// Parse body
$body = file_get_contents('php://input');
$req  = json_decode($body, true);

if (!is_array($req)) {
    http_response_code(400);
    echo json_encode(['ok' => false, 'error' => 'Invalid JSON body.']);
    exit;
}

$action = trim($req['action'] ?? '');

// ---- Auth: stage-token or xcm_auth session -----------------------------------

function pb_is_stage_token_valid(): bool
{
    $token_file = __DIR__ . '/.stage-token';
    if (!file_exists($token_file)) {
        return false;
    }
    $expected = trim(file_get_contents($token_file));
    if (empty($expected)) {
        return false;
    }
    $header = $_SERVER['HTTP_AUTHORIZATION'] ?? $_SERVER['HTTP_X_STAGE_TOKEN'] ?? '';
    if (preg_match('/^Bearer\s+(.+)$/i', $header, $m)) {
        return hash_equals($expected, trim($m[1]));
    }
    return false;
}

$pb_admin_auth = dirname(__DIR__) . '/pb_admin/auth.php';
$pb_admin_conf = dirname(__DIR__) . '/pb_admin/config.php';

$has_session = false;
if (file_exists($pb_admin_conf) && file_exists($pb_admin_auth)) {
    require_once $pb_admin_conf;
    require_once $pb_admin_auth;
    $has_session = true;
}

$authed = pb_is_stage_token_valid() || ($has_session && is_logged_in());
if (!$authed) {
    http_response_code(401);
    echo json_encode(['ok' => false, 'error' => 'Authentication required.']);
    exit;
}

// ---- Block admin-only actions (page-builder should never call these) ---------

require_once dirname(__DIR__) . '/db_bridge/query_guard.php';

if (in_array($action, DB_ADMIN_ACTIONS, true)) {
    http_response_code(403);
    echo json_encode(['ok' => false, 'error' => 'Forbidden action for this endpoint.']);
    exit;
}

// ---- Default db to the pages database ----------------------------------------

if (empty($req['db'])) {
    $req['db'] = 'dev-tools/db-browser/databases/pages';
}

// ---- Dispatch via db_bridge/api.php (internal include) ----------------------

ob_start();
$GLOBALS['_DB_BRIDGE_REQUEST'] = $req;
include dirname(__DIR__) . '/db_bridge/api.php';
$result = ob_get_clean();

echo $result;
