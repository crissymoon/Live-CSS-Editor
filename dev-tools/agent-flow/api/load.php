<?php
/**
 * agent-flow/api/load.php
 * GET ?name=<name> -- returns {ok, flow}
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

define('FLOWS_DIR', __DIR__ . '/../flows');

function jsonErr(string $msg, int $http = 400): void {
    http_response_code($http);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function safeName(string $name): string {
    return preg_replace('/[^a-zA-Z0-9_-]/', '-', $name);
}

$name = safeName(trim($_GET['name'] ?? ''));
if (!$name) jsonErr('Missing flow name', 400);

$path = FLOWS_DIR . '/' . $name . '.json';
if (!file_exists($path)) jsonErr('Flow not found: ' . $name, 404);

$raw = file_get_contents($path);
if ($raw === false) {
    error_log('agent-flow/load.php: read failed for ' . $path);
    jsonErr('Could not read flow file.', 500);
}

$flow = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    error_log('agent-flow/load.php: corrupt JSON in ' . $path);
    jsonErr('Flow file is corrupt: ' . json_last_error_msg(), 500);
}

echo json_encode(['ok' => true, 'name' => $name, 'flow' => $flow]);
