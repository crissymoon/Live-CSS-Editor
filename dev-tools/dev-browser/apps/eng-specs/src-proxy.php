<?php
/**
 * src-proxy.php -- serve WASM/JS build artifacts to the ENG Specs page.
 *
 * Usage:
 *   ?f=render_core.wasm   -> application/wasm
 *   ?f=render_core.js     -> application/javascript
 *
 * Only filenames matching the exact whitelist are served.
 * All other requests get 403.
 */

$ALLOWED = [
    'render_core.wasm' => 'application/wasm',
    'render_core.js'   => 'application/javascript',
];

$file = isset($_GET['f']) ? basename($_GET['f']) : '';

if (!isset($ALLOWED[$file])) {
    http_response_code(403);
    echo 'Forbidden';
    exit;
}

// Path: dev-browser/src/<file>
$src_dir = dirname(dirname(__DIR__)) . DIRECTORY_SEPARATOR . 'src';
$path    = $src_dir . DIRECTORY_SEPARATOR . $file;

if (!file_exists($path)) {
    http_response_code(404);
    echo 'File not found: ' . htmlspecialchars($file);
    exit;
}

header('Content-Type: ' . $ALLOWED[$file]);
header('Content-Length: ' . filesize($path));
header('Cache-Control: no-store');
header('Access-Control-Allow-Origin: *');
readfile($path);
