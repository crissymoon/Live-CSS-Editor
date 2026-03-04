<?php
/**
 * pb_admin/api_proxy.php
 * Thin proxy: receives action requests from the browser JS and forwards them
 * to the xcm_auth server so the Bearer token never touches the client.
 *
 * All errors are written to error_log for console debugging.
 */
require_once __DIR__ . '/auth.php';

header('Content-Type: application/json; charset=utf-8');

$action = $_GET['action'] ?? $_POST['action'] ?? '';

error_log('[pb_admin/api_proxy] action=' . $action);

switch ($action) {

    // ── Health check (unauthenticated) ────────────────────────────────────
    case 'health':
        $resp = xcm_get('/health');
        if ($resp['ok']) {
            echo json_encode(['ok' => true, 'server' => XCMAUTH_BASE_URL, 'data' => $resp['data']]);
        } else {
            echo json_encode(['ok' => false, 'error' => $resp['error']]);
        }
        break;

    // ── Current user info ─────────────────────────────────────────────────
    case 'me':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $resp = xcm_get('/user/me', access_token());
        echo json_encode($resp);
        break;

    // ── Active sessions ───────────────────────────────────────────────────
    case 'sessions':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $resp = xcm_get('/user/sessions', access_token());
        echo json_encode($resp);
        break;

    // ── Audit log ─────────────────────────────────────────────────────────
    case 'audit':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $limit  = max(1, min(100, (int)($_GET['limit']  ?? 20)));
        $offset = max(0, (int)($_GET['offset'] ?? 0));
        $resp = xcm_get('/user/audit?limit=' . $limit . '&offset=' . $offset, access_token());
        echo json_encode($resp);
        break;

    // ── Admin: user list ──────────────────────────────────────────────────
    case 'admin_users':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $limit  = max(1, min(100, (int)($_GET['limit']  ?? 50)));
        $offset = max(0, (int)($_GET['offset'] ?? 0));
        $resp = xcm_get('/admin/users?limit=' . $limit . '&offset=' . $offset, access_token());
        echo json_encode($resp);
        break;
    // ── Admin: create user ───────────────────────────────────────────────
    case 'admin_create_user':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $body = json_decode(file_get_contents('php://input'), true);
        if (!is_array($body)) {
            error_log('[pb_admin/api_proxy] admin_create_user: invalid body');
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'invalid request body']);
            break;
        }
        error_log('[pb_admin/api_proxy] admin_create_user: username=' . ($body['username'] ?? ''));
        $resp = xcm_post('/admin/users', $body, access_token());
        echo json_encode($resp);
        break;

    // ── Admin: update user (role / active) ─────────────────────────────────
    case 'admin_update_user':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'invalid user id']);
            break;
        }
        $body = json_decode(file_get_contents('php://input'), true);
        if (!is_array($body)) {
            error_log('[pb_admin/api_proxy] admin_update_user: invalid body for id=' . $id);
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'invalid request body']);
            break;
        }
        error_log('[pb_admin/api_proxy] admin_update_user: id=' . $id . ' body=' . json_encode($body));
        $resp = xcm_request('PATCH', '/admin/users/' . $id, $body, access_token());
        echo json_encode($resp);
        break;

    // ── Admin: deactivate user ──────────────────────────────────────────────
    case 'admin_deactivate_user':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'invalid user id']);
            break;
        }
        error_log('[pb_admin/api_proxy] admin_deactivate_user: id=' . $id);
        $resp = xcm_request('DELETE', '/admin/users/' . $id, null, access_token());
        echo json_encode($resp);
        break;

    // ── Admin: reactivate user ─────────────────────────────────────────────
    case 'admin_reactivate_user':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $id = (int)($_GET['id'] ?? 0);
        if ($id <= 0) {
            http_response_code(400);
            echo json_encode(['ok' => false, 'error' => 'invalid user id']);
            break;
        }
        error_log('[pb_admin/api_proxy] admin_reactivate_user: id=' . $id);
        $resp = xcm_request('PATCH', '/admin/users/' . $id, ['is_active' => true], access_token());
        echo json_encode($resp);
        break;

    // ── Dev tools: launch db-browser in a Terminal window ────────────────
    case 'launch_db_browser':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $scriptPath = '/Users/mac/Documents/live-css/dev-tools/db-browser/quick-launch.sh';
        if (!file_exists($scriptPath)) {
            echo json_encode(['ok' => false, 'error' => 'quick-launch.sh not found at ' . $scriptPath]);
            break;
        }
        // Open a new macOS Terminal window and run the interactive quick-launch script
        $escapedPath = escapeshellarg($scriptPath);
        $cmd = 'osascript -e \'tell application "Terminal" to do script ' . escapeshellarg($scriptPath) . '\' > /dev/null 2>&1 &';
        exec('osascript -e \'tell application "Terminal" to do script "' . addslashes($scriptPath) . '" activate\' > /dev/null 2>&1 &');
        error_log('[pb_admin/api_proxy] launch_db_browser: opened Terminal with ' . $scriptPath);
        echo json_encode(['ok' => true, 'msg' => 'Terminal opened']);
        break;

    // ── Dev tools: db-browser status ─────────────────────────────────────
    case 'db_browser_status':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $binPath    = '/Users/mac/Documents/live-css/dev-tools/db-browser/build/bin/db-browser';
        $scriptPath = '/Users/mac/Documents/live-css/dev-tools/db-browser/quick-launch.sh';
        $dbFiles    = [];
        $dbScan     = [
            '/Users/mac/Documents/live-css/xcm-editor.db',
            '/Users/mac/Documents/live-css/debug-tool/db/errors.db',
            '/Users/mac/Documents/live-css/xcm_auth/xcm_auth_dev.db',
            '/Users/mac/Documents/live-css/vscode-bridge/data/projects.db',
            '/Users/mac/Documents/live-css/ai/data/phrases.db',
        ];
        foreach ($dbScan as $p) {
            if (file_exists($p)) {
                $dbFiles[] = ['path' => $p, 'name' => basename($p), 'size' => filesize($p)];
            }
        }
        echo json_encode([
            'ok'         => true,
            'binary_ok'  => file_exists($binPath),
            'script_ok'  => file_exists($scriptPath),
            'binary_path'=> $binPath,
            'db_files'   => $dbFiles,
        ]);
        break;

    // ── Database bridge (admin.*  and pages.* actions) ────────────────────
    // All db.* requests are forwarded to db_bridge/api.php internally:
    //   POST ?action=db&db=<key>&db_action=<action>&params=<json>
    //
    // The xcm_auth session already validated above via is_logged_in();
    // db_bridge/api.php re-checks is_logged_in() for the admin gate.
    case 'db':
        if (!is_logged_in()) {
            http_response_code(401);
            echo json_encode(['ok' => false, 'error' => 'not authenticated']);
            break;
        }
        $db_bridge = __DIR__ . '/../db_bridge/api.php';
        if (!file_exists($db_bridge)) {
            http_response_code(503);
            echo json_encode(['ok' => false, 'error' => 'db_bridge not available']);
            break;
        }
        // Rebuild the request as the internal API expects it
        $raw_body  = file_get_contents('php://input');
        $body      = json_decode($raw_body, true);
        $db_action = $body['action'] ?? ($_GET['db_action'] ?? '');
        $db_params = $body['params'] ?? [];
        $db_name   = $body['db']     ?? ($_GET['db'] ?? '');

        $inner_req = [
            'action' => $db_action,
            'db'     => $db_name,
            'params' => $db_params,
        ];

        // Call db_bridge/api.php as an include with a spoofed request body.
        // We isolate it in a closure so it can't pollute our scope.
        $db_result = (static function (string $bridge_path, array $inner) {
            ob_start();
            // Override php://input reading inside api.php by pre-parsing here
            // and injecting via $_DB_BRIDGE_REQUEST superglobal (api.php checks for this).
            $GLOBALS['_DB_BRIDGE_REQUEST'] = $inner;
            include $bridge_path;
            return ob_get_clean();
        })($db_bridge, $inner_req);

        echo $db_result;
        break;

    default:
        http_response_code(400);
        echo json_encode(['ok' => false, 'error' => 'unknown action: ' . htmlspecialchars($action)]);
}
