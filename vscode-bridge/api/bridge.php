<?php
/**
 * vscode-bridge/api/bridge.php
 * HTTP endpoint that connects the browser session to the MCP server.
 *
 * Routes:
 *   POST ?action=push_session      -- browser pushes its current CSS/HTML state
 *   GET  ?action=poll_changes      -- browser asks if MCP wrote any file changes
 *   GET  ?action=session           -- read the stored session (debug/testing)
 *
 * All errors are reported to error_log() and return JSON with success:false
 * so the browser client can log them to the console.
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// When this file is served from src-tauri/www/vscode-bridge/api/, the real
// project root is two levels above DOCUMENT_ROOT (www/ -> src-tauri/ -> root).
// Fall back to __DIR__/../data if DOCUMENT_ROOT is not available (CLI/tests).
define('BRIDGE_DATA_DIR',
    (isset($_SERVER['DOCUMENT_ROOT']) && $_SERVER['DOCUMENT_ROOT'])
        ? rtrim($_SERVER['DOCUMENT_ROOT'], '/\\') . '/../../vscode-bridge/data'
        : __DIR__ . '/../data'
);
define('SESSION_FILE',           BRIDGE_DATA_DIR . '/session.json');
define('CHANGES_FILE',           BRIDGE_DATA_DIR . '/pending-changes.json');
define('CHANGES_ACK_FILE',       BRIDGE_DATA_DIR . '/changes-ack.json');
define('REFRESH_FILE',           BRIDGE_DATA_DIR . '/refresh-signal.json');
define('WIREFRAME_FILE',         BRIDGE_DATA_DIR . '/wireframe.json');
define('WIREFRAME_CHANGES_FILE', BRIDGE_DATA_DIR . '/wireframe-changes.json');
define('WIREFRAME_ACK_FILE',     BRIDGE_DATA_DIR . '/wireframe-ack.json');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
function ensureDataDir(): void {
    if (!is_dir(BRIDGE_DATA_DIR)) {
        if (!mkdir(BRIDGE_DATA_DIR, 0755, true)) {
            error_log('[VSCodeBridge] Could not create data dir: ' . BRIDGE_DATA_DIR);
        }
    }
}

function jsonOut(array $data, int $code = 200): void {
    http_response_code($code);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function readJsonFile(string $path): ?array {
    if (!file_exists($path)) return null;
    $raw = file_get_contents($path);
    if ($raw === false) {
        error_log('[VSCodeBridge] Failed to read: ' . $path);
        return null;
    }
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[VSCodeBridge] JSON decode failed for ' . $path . ': ' . json_last_error_msg());
        return null;
    }
    return $data;
}

function writeJsonFile(string $path, array $data): bool {
    ensureDataDir();
    $json = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
    $ok   = file_put_contents($path, $json, LOCK_EX);
    if ($ok === false) {
        error_log('[VSCodeBridge] Failed to write: ' . $path);
        return false;
    }
    return true;
}

function readBody(): array {
    $raw = file_get_contents('php://input');
    if (empty($raw)) return [];
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[VSCodeBridge] readBody: invalid JSON - ' . json_last_error_msg());
        return [];
    }
    return $data ?? [];
}

// ---------------------------------------------------------------------------
// Router
// ---------------------------------------------------------------------------
try {
    ensureDataDir();
    $action = $_GET['action'] ?? '';

    // -- POST ?action=push_session
    // Browser sends current editor state so MCP/Copilot can read it
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'push_session') {
        $body = readBody();

        if (empty($body)) {
            error_log('[VSCodeBridge] push_session: empty body');
            jsonOut(['success' => false, 'error' => 'Empty body'], 400);
        }

        $session = [
            'css'         => $body['css']         ?? '',
            'html'        => $body['html']         ?? '',
            'activeSheet' => $body['activeSheet']  ?? '',
            'projects'    => $body['projects']     ?? [],
            'source'      => 'browser',
            'syncedAt'    => date('Y-m-d H:i:s'),
        ];

        $ok = writeJsonFile(SESSION_FILE, $session);
        if (!$ok) {
            jsonOut(['success' => false, 'error' => 'Could not write session file'], 500);
        }

        error_log('[VSCodeBridge] push_session: stored session for sheet "' . ($session['activeSheet'] ?: 'unknown') . '"');
        jsonOut(['success' => true, 'syncedAt' => $session['syncedAt']]);
    }

    // -- GET ?action=poll_changes
    // Browser asks: "did Copilot change any file since I last checked?"
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'poll_changes') {
        $changes = readJsonFile(CHANGES_FILE);
        if (!$changes) {
            jsonOut(['success' => true, 'hasChanges' => false]);
        }

        // Check if browser already acknowledged this change
        $ack        = readJsonFile(CHANGES_ACK_FILE);
        $lastAck    = $ack['updatedAt'] ?? '';
        $changeTime = $changes['updatedAt'] ?? '';

        if ($lastAck === $changeTime) {
            jsonOut(['success' => true, 'hasChanges' => false]);
        }

        error_log('[VSCodeBridge] poll_changes: pending change for "' . ($changes['file'] ?? '?') . '" at ' . $changeTime);
        jsonOut([
            'success'    => true,
            'hasChanges' => true,
            'file'       => $changes['file']      ?? null,
            'updatedAt'  => $changes['updatedAt'] ?? null,
            'by'         => $changes['by']        ?? 'vscode-copilot',
        ]);
    }

    // -- POST ?action=ack_changes
    // Browser acknowledges it has reloaded the changed file
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'ack_changes') {
        $changes = readJsonFile(CHANGES_FILE);
        if ($changes) {
            writeJsonFile(CHANGES_ACK_FILE, [
                'updatedAt'   => $changes['updatedAt'] ?? '',
                'ackedAt'     => date('Y-m-d H:i:s'),
            ]);
        }
        jsonOut(['success' => true]);
    }

    // -- GET ?action=session  (debug read)
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'session') {
        $session = readJsonFile(SESSION_FILE);
        if (!$session) {
            jsonOut(['success' => false, 'error' => 'No session on file']);
        }
        jsonOut(['success' => true, 'data' => $session]);
    }

    // -- POST ?action=request_refresh
    // MCP / CLI signals the browser to do a full page reload
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'request_refresh') {
        $ts = date('Y-m-d H:i:s');
        $ok = writeJsonFile(REFRESH_FILE, ['refresh' => true, 'requestedAt' => $ts]);
        if (!$ok) {
            jsonOut(['success' => false, 'error' => 'Could not write refresh signal'], 500);
        }
        error_log('[VSCodeBridge] request_refresh: signal written at ' . $ts);
        jsonOut(['success' => true, 'requestedAt' => $ts]);
    }

    // -- GET ?action=poll_refresh
    // Browser polls: should I reload?
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'poll_refresh') {
        $sig = readJsonFile(REFRESH_FILE);
        if (!$sig || empty($sig['refresh'])) {
            jsonOut(['success' => true, 'shouldRefresh' => false]);
        }
        // Clear the signal immediately so a second fast poll does not double-reload
        writeJsonFile(REFRESH_FILE, ['refresh' => false, 'clearedAt' => date('Y-m-d H:i:s')]);
        error_log('[VSCodeBridge] poll_refresh: signalling browser to reload (was set at ' . ($sig['requestedAt'] ?? '?') . ')');
        jsonOut(['success' => true, 'shouldRefresh' => true, 'requestedAt' => $sig['requestedAt'] ?? '']);
    }

    // -- POST ?action=push_wireframe
    // Browser sends the current wireframe JSON so MCP/Copilot can read it
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'push_wireframe') {
        $body = readBody();
        if (empty($body)) {
            error_log('[VSCodeBridge] push_wireframe: empty body');
            jsonOut(['success' => false, 'error' => 'Empty body'], 400);
        }
        if (!isset($body['elements']) || !is_array($body['elements'])) {
            jsonOut(['success' => false, 'error' => 'Missing elements array'], 400);
        }
        $payload = [
            'version'     => $body['version']     ?? 1,
            'nextId'      => $body['nextId']      ?? 1,
            'nextGuideId' => $body['nextGuideId'] ?? 1,
            'elements'    => $body['elements'],
            'guides'      => $body['guides']      ?? [],
            'pushedAt'    => date('Y-m-d H:i:s'),
            'source'      => 'browser',
        ];
        $ok = writeJsonFile(WIREFRAME_FILE, $payload);
        if (!$ok) {
            jsonOut(['success' => false, 'error' => 'Could not write wireframe file'], 500);
        }
        error_log('[VSCodeBridge] push_wireframe: stored ' . count($body['elements']) . ' element(s)');
        jsonOut(['success' => true, 'pushedAt' => $payload['pushedAt']]);
    }

    // -- GET ?action=read_wireframe
    // Read stored wireframe JSON (for MCP debug / browser rehydration)
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'read_wireframe') {
        $wf = readJsonFile(WIREFRAME_FILE);
        if (!$wf) {
            jsonOut(['success' => false, 'error' => 'No wireframe on file']);
        }
        jsonOut(['success' => true, 'data' => $wf]);
    }

    // -- POST ?action=update_wireframe
    // MCP/Copilot pushes changes to the wireframe; browser picks them up via poll
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'update_wireframe') {
        $body = readBody();
        if (empty($body) || !isset($body['elements'])) {
            jsonOut(['success' => false, 'error' => 'Missing elements array'], 400);
        }
        $payload = [
            'version'     => $body['version']     ?? 1,
            'nextId'      => $body['nextId']      ?? 1,
            'nextGuideId' => $body['nextGuideId'] ?? 1,
            'elements'    => $body['elements'],
            'guides'      => $body['guides']      ?? [],
            'updatedAt'   => date('Y-m-d H:i:s'),
            'source'      => 'copilot',
        ];
        // Write the new canonical wireframe
        $ok = writeJsonFile(WIREFRAME_FILE, $payload);
        if (!$ok) {
            jsonOut(['success' => false, 'error' => 'Could not write wireframe file'], 500);
        }
        // Write change signal so browser poll picks it up
        writeJsonFile(WIREFRAME_CHANGES_FILE, [
            'updatedAt' => $payload['updatedAt'],
            'by'        => 'vscode-copilot',
        ]);
        error_log('[VSCodeBridge] update_wireframe: wrote ' . count($body['elements']) . ' element(s) from Copilot');
        jsonOut(['success' => true, 'updatedAt' => $payload['updatedAt']]);
    }

    // -- GET ?action=poll_wireframe_changes
    // Browser asks: did Copilot update the wireframe since I last checked?
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'poll_wireframe_changes') {
        $changes = readJsonFile(WIREFRAME_CHANGES_FILE);
        if (!$changes) {
            jsonOut(['success' => true, 'hasChanges' => false]);
        }
        $ack        = readJsonFile(WIREFRAME_ACK_FILE);
        $lastAck    = $ack['updatedAt'] ?? '';
        $changeTime = $changes['updatedAt'] ?? '';
        if ($lastAck === $changeTime) {
            jsonOut(['success' => true, 'hasChanges' => false]);
        }
        // Include the full wireframe state so the browser can apply it in one round trip
        $wf = readJsonFile(WIREFRAME_FILE);
        error_log('[VSCodeBridge] poll_wireframe_changes: pending update at ' . $changeTime);
        jsonOut([
            'success'    => true,
            'hasChanges' => true,
            'state'      => $wf,
            'updatedAt'  => $changeTime,
            'by'         => $changes['by'] ?? 'vscode-copilot',
        ]);
    }

    // -- POST ?action=ack_wireframe
    // Browser acknowledges it has applied the wireframe update
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'ack_wireframe') {
        $changes = readJsonFile(WIREFRAME_CHANGES_FILE);
        if ($changes) {
            writeJsonFile(WIREFRAME_ACK_FILE, [
                'updatedAt' => $changes['updatedAt'] ?? '',
                'ackedAt'   => date('Y-m-d H:i:s'),
            ]);
        }
        jsonOut(['success' => true]);
    }

    // Fallthrough
    error_log('[VSCodeBridge] Unhandled: ' . $_SERVER['REQUEST_METHOD'] . ' ?action=' . $action);
    jsonOut(['success' => false, 'error' => 'Unknown action "' . $action . '"'], 404);

} catch (Throwable $e) {
    error_log('[VSCodeBridge] Uncaught: ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
    jsonOut(['success' => false, 'error' => 'Internal error', 'detail' => $e->getMessage()], 500);
}
