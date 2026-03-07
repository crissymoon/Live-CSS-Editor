<?php
/**
 * debug-tool/api/index.php
 * REST API for the debug/error tracking tool.
 *
 * Routes:
 *   POST   /debug-tool/api/          -- create a new error ticket
 *   GET    /debug-tool/api/          -- list tickets (supports query filters)
 *   GET    /debug-tool/api/?id=X     -- get single ticket
 *   PATCH  /debug-tool/api/?id=X     -- update status / ai_analysis
 *   DELETE /debug-tool/api/?id=X     -- delete a ticket
 *   GET    /debug-tool/api/?action=stats       -- summary stats
 *   POST   /debug-tool/api/?action=analyze&id=X -- run AI analysis on ticket
 */

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------
header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PATCH, DELETE, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Debug-Key');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

require_once __DIR__ . '/db.php';
require_once __DIR__ . '/../ai/analyze.php';

// Optional API key protection - set DEBUG_API_KEY env var or leave blank to skip
$apiKey = getenv('DEBUG_API_KEY') ?: '';
if ($apiKey !== '') {
    $provided = $_SERVER['HTTP_X_DEBUG_KEY'] ?? '';
    if ($provided !== $apiKey) {
        error_log('[DebugAPI] Unauthorized request from ' . ($_SERVER['REMOTE_ADDR'] ?? 'unknown'));
        http_response_code(401);
        echo json_encode(['success' => false, 'error' => 'Unauthorized']);
        exit;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
function jsonOut(array $data, int $code = 200): void {
    http_response_code($code);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
    exit;
}

function readBody(): array {
    $raw = file_get_contents('php://input');
    if (empty($raw)) return [];
    $decoded = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[DebugAPI] readBody: invalid JSON - ' . json_last_error_msg());
        return [];
    }
    return $decoded ?? [];
}

// ---------------------------------------------------------------------------
// Router
// ---------------------------------------------------------------------------
try {
    $db     = new DebugDB();
    $method = $_SERVER['REQUEST_METHOD'];
    $action = $_GET['action'] ?? '';
    $id     = trim($_GET['id'] ?? '');

    // -- GET /api/?action=stats
    if ($method === 'GET' && $action === 'stats') {
        jsonOut($db->stats());
    }

    // -- POST /api/?action=analyze&id=X
    if ($method === 'POST' && $action === 'analyze') {
        if ($id === '') {
            jsonOut(['success' => false, 'error' => 'id is required'], 400);
        }
        $result = $db->getError($id);
        if (!$result['success']) {
            jsonOut(['success' => false, 'error' => 'Ticket not found'], 404);
        }
        $analysis = analyzeErrorWithAI($result['data']);
        if ($analysis['success']) {
            $db->updateError($id, ['ai_analysis' => $analysis['analysis']]);
        }
        jsonOut($analysis);
    }

    // -- GET /api/?id=X  (single)
    if ($method === 'GET' && $id !== '') {
        jsonOut($db->getError($id));
    }

    // -- GET /api/  (list)
    if ($method === 'GET') {
        $filters = [
            'level'  => $_GET['level']  ?? '',
            'status' => $_GET['status'] ?? '',
            'source' => $_GET['source'] ?? '',
            'search' => $_GET['search'] ?? '',
            'limit'  => $_GET['limit']  ?? 50,
            'offset' => $_GET['offset'] ?? 0,
            'order'  => $_GET['order']  ?? 'desc',
        ];
        jsonOut($db->listErrors($filters));
    }

    // -- POST /api/  (create)
    if ($method === 'POST') {
        $body = readBody();
        if (empty($body)) {
            // Fallback: try $_POST for form submissions
            $body = $_POST;
        }
        if (empty($body)) {
            error_log('[DebugAPI] POST with empty body');
            jsonOut(['success' => false, 'error' => 'Request body is empty or not valid JSON'], 400);
        }
        $result = $db->insertError($body);
        jsonOut($result, $result['success'] ? 201 : 422);
    }

    // -- PATCH /api/?id=X  (update)
    if ($method === 'PATCH') {
        if ($id === '') {
            jsonOut(['success' => false, 'error' => 'id is required'], 400);
        }
        $body = readBody();
        if (empty($body)) {
            error_log('[DebugAPI] PATCH with empty body for id=' . $id);
            jsonOut(['success' => false, 'error' => 'Request body is empty'], 400);
        }
        jsonOut($db->updateError($id, $body));
    }

    // -- DELETE /api/?id=X
    if ($method === 'DELETE') {
        if ($id === '') {
            jsonOut(['success' => false, 'error' => 'id is required'], 400);
        }
        jsonOut($db->deleteError($id));
    }

    // Fallthrough
    error_log('[DebugAPI] Unhandled route: ' . $method . ' action=' . $action . ' id=' . $id);
    jsonOut(['success' => false, 'error' => 'Unknown route'], 404);

} catch (Throwable $e) {
    error_log('[DebugAPI] Uncaught exception: ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
    jsonOut(['success' => false, 'error' => 'Internal server error', 'detail' => $e->getMessage()], 500);
}
