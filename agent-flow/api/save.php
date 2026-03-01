<?php
/**
 * agent-flow/api/save.php
 * POST JSON {name, flow} -- saves flow to flows/<name>.json
 * DELETE ?delete=<name>  -- removes a saved flow
 *
 * Returns {ok, error?}
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

define('FLOWS_DIR', __DIR__ . '/../flows');

function jsonErr(string $msg, int $http = 400): void {
    http_response_code($http);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function ensureFlowsDir(): void {
    if (!is_dir(FLOWS_DIR)) {
        if (!mkdir(FLOWS_DIR, 0755, true)) {
            error_log('agent-flow/save.php: could not create flows/ dir');
            jsonErr('Could not create flows directory.', 500);
        }
    }
}

function safeName(string $name): string {
    return preg_replace('/[^a-zA-Z0-9_-]/', '-', $name);
}

$method = $_SERVER['REQUEST_METHOD'];

if ($method === 'POST') {
    ensureFlowsDir();
    $raw = file_get_contents('php://input');
    if (!$raw) jsonErr('Empty body', 400);

    $body = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) jsonErr('Invalid JSON: ' . json_last_error_msg(), 400);

    $name = safeName(trim($body['name'] ?? ''));
    if (!$name) jsonErr('Missing or invalid flow name', 400);

    $flow = $body['flow'] ?? null;
    if (!$flow) jsonErr('Missing flow data', 400);

    $path = FLOWS_DIR . '/' . $name . '.json';
    $json = json_encode($flow, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE);

    if (file_put_contents($path, $json) === false) {
        error_log('agent-flow/save.php: write failed for ' . $path);
        jsonErr('Could not write flow file.', 500);
    }

    echo json_encode(['ok' => true, 'name' => $name]);

} elseif ($method === 'DELETE') {
    $name = safeName(trim($_GET['delete'] ?? ''));
    if (!$name) jsonErr('Missing flow name', 400);

    $path = FLOWS_DIR . '/' . $name . '.json';
    if (!file_exists($path)) jsonErr('Flow not found: ' . $name, 404);

    if (!unlink($path)) {
        error_log('agent-flow/save.php: unlink failed for ' . $path);
        jsonErr('Could not delete flow.', 500);
    }

    echo json_encode(['ok' => true]);

} else {
    jsonErr('Method not allowed', 405);
}
