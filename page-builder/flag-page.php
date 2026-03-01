<?php
/**
 * page-builder/flag-page.php
 *
 * POST-only endpoint. Sets a per-page flag (currently: blocked from staging).
 * Flags are stored in  pages/{name}/.flags.json  (dot prefix keeps them out of
 * the public site; they are never copied to the deploy folder).
 *
 * Request body (JSON):
 *   { "page": "demo", "blocked": true|false }
 *
 * Response (JSON):
 *   { "ok": true, "page": "demo", "blocked": true }
 *
 * Security:
 *   - POST only (no state changes via GET)
 *   - page name is validated against a strict pattern (a-z0-9_-)
 *   - path traversal is blocked by that same validation
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

function flagErr(string $msg, int $code = 400): void {
    error_log('[flag-page] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    flagErr('POST required', 405);
}

$raw  = file_get_contents('php://input');
$body = json_decode($raw ?: '{}', true);

if (json_last_error() !== JSON_ERROR_NONE) {
    flagErr('Invalid JSON body: ' . json_last_error_msg());
}

$page    = trim($body['page'] ?? '');
$blocked = isset($body['blocked']) ? (bool) $body['blocked'] : null;

if (!$page || !preg_match('/^[a-z0-9_-]+$/i', $page)) {
    flagErr('Missing or invalid page name (a-z 0-9 _ - only)');
}
if ($blocked === null) {
    flagErr('Missing "blocked" field (true or false)');
}

$pagesRoot = __DIR__ . '/pages';
$pageDir   = $pagesRoot . '/' . $page;

if (!is_dir($pageDir)) {
    flagErr('Page not found: ' . $page, 404);
}

// Read existing flags so we do not clobber future fields
$flagsFile = $pageDir . '/.flags.json';
$flags     = [];
if (file_exists($flagsFile)) {
    $raw2 = file_get_contents($flagsFile);
    if ($raw2 !== false) {
        $parsed = json_decode($raw2, true);
        if (is_array($parsed)) {
            $flags = $parsed;
        } else {
            error_log('[flag-page] .flags.json is not an array for page: ' . $page . ', starting fresh');
        }
    }
}

$flags['blocked']    = $blocked;
$flags['updated_at'] = date('c');

$written = file_put_contents($flagsFile, json_encode($flags, JSON_PRETTY_PRINT));
if ($written === false) {
    flagErr('Failed to write .flags.json for page: ' . $page, 500);
}

error_log('[flag-page] page=' . $page . ' blocked=' . ($blocked ? 'true' : 'false'));
echo json_encode(['ok' => true, 'page' => $page, 'blocked' => $blocked]);
