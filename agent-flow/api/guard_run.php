<?php
/**
 * agent-flow/api/guard_run.php
 *
 * Lightweight PHP proxy for the prompt injection guard server (Python, port 8765).
 * Forwards classify requests from the browser (same-origin) to the guard server
 * and returns the JSON response unchanged.
 *
 * POST JSON: { text: string, guardUrl?: string }
 *   guardUrl defaults to http://localhost:8765/classify
 *
 * Returns the guard server's JSON response, or a structured error object:
 *   { ok: false, error: string, flagged: false, label: "clean", confidence: 0 }
 *
 * Fallback: if the guard server is unreachable the endpoint returns ok:false with
 *   flagged:false so callers can fail-open without blocking the UI.
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

function guardErr(string $msg, int $http = 400): void {
    http_response_code($http);
    error_log('agent-flow/guard_run.php: ' . $msg);
    echo json_encode([
        'ok'         => false,
        'error'      => $msg,
        'flagged'    => false,
        'label'      => 'clean',
        'confidence' => 0,
    ]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    guardErr('Method not allowed -- use POST', 405);
}

$raw = file_get_contents('php://input');
if (!$raw) {
    guardErr('Empty request body', 400);
}

$body = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    guardErr('Invalid JSON body: ' . json_last_error_msg(), 400);
}

$text     = $body['text']     ?? null;
$guardUrl = $body['guardUrl'] ?? 'http://localhost:8765/classify';

if ($text === null || !is_string($text)) {
    guardErr('Missing required field: text', 400);
}

// Validate guardUrl is a reasonable localhost/internal URL so this proxy
// cannot be abused to make arbitrary outbound requests.
$parsed = parse_url($guardUrl);
$host   = $parsed['host'] ?? '';
$allowedHosts = ['localhost', '127.0.0.1', '::1'];
if (!in_array($host, $allowedHosts, true)) {
    guardErr('guardUrl host must be localhost or 127.0.0.1 for security', 403);
}

$payload = json_encode(['text' => $text]);

$ch = curl_init($guardUrl);
curl_setopt_array($ch, [
    CURLOPT_POST           => true,
    CURLOPT_POSTFIELDS     => $payload,
    CURLOPT_RETURNTRANSFER => true,
    CURLOPT_TIMEOUT        => 10,
    CURLOPT_CONNECTTIMEOUT => 5,
    CURLOPT_HTTPHEADER     => ['Content-Type: application/json'],
]);

$resp     = curl_exec($ch);
$curlErr  = curl_error($ch);
$httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

if ($curlErr) {
    error_log('agent-flow/guard_run.php: cURL error: ' . $curlErr);
    guardErr('guard server unreachable: ' . $curlErr, 502);
}

if ($httpCode >= 400) {
    error_log('agent-flow/guard_run.php: guard server HTTP ' . $httpCode . ': ' . substr($resp, 0, 200));
    guardErr('guard server returned HTTP ' . $httpCode . ': ' . substr($resp, 0, 120), 502);
}

// Validate guard response is JSON before forwarding.
$decoded = json_decode($resp, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    error_log('agent-flow/guard_run.php: guard server non-JSON response: ' . substr($resp, 0, 200));
    guardErr('guard server returned non-JSON: ' . substr($resp, 0, 120), 502);
}

// Pass the guard server response straight through to the browser.
echo $resp;
