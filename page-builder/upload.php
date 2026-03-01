<?php
/**
 * page-builder/upload.php
 *
 * Accepts a multipart POST with a single image file and saves it to
 * page-builder/assets/. Returns JSON with the public URL.
 *
 * POST fields:
 *   file  - the uploaded file (image/*)
 *   page  - (optional) page name, used only for logging
 *
 * Response:
 *   { "ok": true,  "url": "/page-builder/assets/filename.ext" }
 *   { "ok": false, "error": "..." }
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

function uploadErr(string $msg, int $code = 400): void {
    error_log('[page-builder/upload] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    uploadErr('Only POST allowed', 405);
}

if (empty($_FILES['file'])) {
    uploadErr('No file field in request');
}

$f = $_FILES['file'];

if ($f['error'] !== UPLOAD_ERR_OK) {
    $phpErrors = [
        UPLOAD_ERR_INI_SIZE   => 'File exceeds upload_max_filesize',
        UPLOAD_ERR_FORM_SIZE  => 'File exceeds MAX_FILE_SIZE',
        UPLOAD_ERR_PARTIAL    => 'File was only partially uploaded',
        UPLOAD_ERR_NO_FILE    => 'No file was uploaded',
        UPLOAD_ERR_NO_TMP_DIR => 'Missing temp folder',
        UPLOAD_ERR_CANT_WRITE => 'Failed to write file to disk',
        UPLOAD_ERR_EXTENSION  => 'A PHP extension stopped the upload',
    ];
    uploadErr($phpErrors[$f['error']] ?? ('PHP upload error code ' . $f['error']));
}

// Validate MIME type (images only)
$allowed = ['image/jpeg', 'image/png', 'image/gif', 'image/webp', 'image/svg+xml', 'image/avif'];
$finfo   = finfo_open(FILEINFO_MIME_TYPE);
$mime    = finfo_file($finfo, $f['tmp_name']);
finfo_close($finfo);

if (!in_array($mime, $allowed, true)) {
    uploadErr('File type not allowed: ' . $mime . '. Allowed: ' . implode(', ', $allowed));
}

$extMap = [
    'image/jpeg'    => 'jpg',
    'image/png'     => 'png',
    'image/gif'     => 'gif',
    'image/webp'    => 'webp',
    'image/svg+xml' => 'svg',
    'image/avif'    => 'avif',
];
$ext = $extMap[$mime];

// Sanitize original filename and append hash for uniqueness
$orig     = pathinfo($f['name'], PATHINFO_FILENAME);
$safe     = preg_replace('/[^a-zA-Z0-9_-]/', '-', $orig);
$safe     = substr($safe, 0, 48);
$filename = $safe . '-' . substr(md5_file($f['tmp_name']), 0, 8) . '.' . $ext;

$assetsDir = __DIR__ . '/assets';
if (!is_dir($assetsDir)) {
    if (!mkdir($assetsDir, 0755, true)) {
        uploadErr('Cannot create assets directory', 500);
    }
}

$dest = $assetsDir . '/' . $filename;

if (!move_uploaded_file($f['tmp_name'], $dest)) {
    uploadErr('Could not move uploaded file to: ' . $dest, 500);
}

$page = $_POST['page'] ?? 'unknown';
error_log('[page-builder/upload] Saved "' . $filename . '" (page: ' . $page . ', mime: ' . $mime . ')');

echo json_encode([
    'ok'  => true,
    'url' => '/page-builder/assets/' . $filename,
    'filename' => $filename,
]);
