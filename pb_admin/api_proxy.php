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
    default:
        http_response_code(400);
        echo json_encode(['ok' => false, 'error' => 'unknown action: ' . htmlspecialchars($action)]);
}
