<?php
/**
 * page-builder/save.php
 *
 * Receives an edit action from pb-editor.js and persists it to
 * pages/{page}/overrides.json. The change is already live in the
 * browser DOM at this point - this just makes it survive a rebuild.
 *
 * POST body (JSON):
 *   {
 *     "page":  "demo",
 *     "id":    "hero-heading",
 *     "prop":  "color",          // CSS property name OR "text"
 *     "value": "#ff0000"
 *   }
 *
 * Response JSON: { "ok": true } or { "ok": false, "error": "..." }
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

function saveErr(string $msg, int $code = 400): void {
    error_log('[page-builder/save] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function saveOk(array $extra = []): void {
    echo json_encode(array_merge(['ok' => true], $extra));
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    saveErr('Only POST allowed', 405);
}

// Read body
$raw = file_get_contents('php://input');
if (empty($raw)) {
    saveErr('Empty request body');
}

$body = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    saveErr('Invalid JSON: ' . json_last_error_msg());
}

$page  = $body['page']  ?? '';
$id    = $body['id']    ?? '';
$prop  = $body['prop']  ?? '';
$value = $body['value'] ?? null;

// Validation
if (!$page || !preg_match('/^[a-z0-9_-]+$/i', $page)) {
    saveErr('Invalid or missing "page"');
}
if (!$id || !preg_match('/^[a-z0-9_-]+$/i', $id)) {
    saveErr('Invalid or missing "id"');
}
if (!$prop || !preg_match('/^[a-zA-Z_-]+$/', $prop)) {
    saveErr('Invalid or missing "prop"');
}
if ($value === null) {
    saveErr('Missing "value"');
}

// Sanitize value - allow CSS values and text content, block script injection
$value = strip_tags((string)$value);
if (strlen($value) > 2000) {
    saveErr('Value too long');
}

$pageDir       = __DIR__ . '/pages/' . $page;
$overridesFile = $pageDir . '/overrides.json';

if (!is_dir($pageDir)) {
    saveErr('Page not found: ' . $page, 404);
}

// Load existing overrides
$overrides = [];
if (file_exists($overridesFile)) {
    $existing = file_get_contents($overridesFile);
    if ($existing !== false) {
        $decoded = json_decode($existing, true);
        if (is_array($decoded)) {
            $overrides = $decoded;
        } else {
            error_log('[page-builder/save] overrides.json was malformed - starting fresh for page: ' . $page);
        }
    }
}

// Apply the new value
if (!isset($overrides[$id])) {
    $overrides[$id] = [];
}
$overrides[$id][$prop] = $value;

// Write back atomically using a temp file
$json = json_encode($overrides, JSON_PRETTY_PRINT | JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
if ($json === false) {
    saveErr('JSON encode failed', 500);
}

$tmp = $overridesFile . '.tmp';
if (file_put_contents($tmp, $json, LOCK_EX) === false) {
    saveErr('Cannot write overrides file (check permissions): ' . $tmp, 500);
}

if (!rename($tmp, $overridesFile)) {
    // rename failed - try direct write
    if (file_put_contents($overridesFile, $json, LOCK_EX) === false) {
        saveErr('Cannot persist overrides for page: ' . $page, 500);
    }
}

error_log('[page-builder/save] Saved ' . $id . '.' . $prop . ' = "' . $value . '" for page "' . $page . '"');
saveOk(['id' => $id, 'prop' => $prop, 'page' => $page]);
