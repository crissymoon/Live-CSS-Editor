<?php
/**
 * pb_admin/router.php
 * Router script for PHP's built-in dev server.
 *
 * Usage (from repo root):
 *   php -S localhost:8080 pb_admin/router.php
 *
 * Rules:
 *  - Requests under /pb_admin/ are served normally from the repo root.
 *  - Bare admin filenames (/dashboard.php, /login.php, etc.) that would
 *    otherwise fall through to the Crissy Style Tool are redirected to
 *    /pb_admin/<filename> so bookmarks and typos land in the right place.
 *  - Everything else is served normally (static files, root index.php, etc.).
 *
 * This file is ONLY used for the PHP built-in dev server.
 * It is NOT loaded by Apache/nginx.
 */

$uri  = $_SERVER['REQUEST_URI'];
$path = parse_url($uri, PHP_URL_PATH);

error_log('[router] incoming: ' . $uri);

// ---- Redirect bare admin filenames to /pb_admin/ ----------------------------
// Catches /dashboard.php, /login.php, /logout.php, /setup.php, /api_proxy.php
// so they land in the right place instead of falling through to root index.php.
$adminFiles = [
    'dashboard.php',
    'login.php',
    'logout.php',
    'setup.php',
    'api_proxy.php',
];

foreach ($adminFiles as $f) {
    if ($path === '/' . $f) {
        $qs  = isset($_SERVER['QUERY_STRING']) && $_SERVER['QUERY_STRING'] !== ''
             ? '?' . $_SERVER['QUERY_STRING']
             : '';
        $dest = '/pb_admin/' . $f . $qs;
        error_log('[router] bare admin path ' . $path . ' -> redirect ' . $dest);
        header('Location: ' . $dest, true, 302);
        exit;
    }
}

// ---- Redirect bare /pb_admin (no trailing slash) ----------------------------
if ($path === '/pb_admin') {
    error_log('[router] /pb_admin -> redirect /pb_admin/');
    header('Location: /pb_admin/', true, 301);
    exit;
}

// ---- Route /pb_admin/* to the correct file ----------------------------------
// PHP's built-in server needs an explicit file path when a router is active.
if (strpos($path, '/pb_admin/') === 0) {
    $file = __DIR__ . '/../' . ltrim($path, '/');
    // Strip query string from file path if any leaked in
    $file = strtok($file, '?');
    if (is_file($file)) {
        error_log('[router] /pb_admin/* -> serve file ' . realpath($file));
        // Return false so PHP serves the file natively (handles PHP parsing).
        return false;
    }
    // File not found inside pb_admin -- log and fall through to 404.
    error_log('[router] /pb_admin/* file not found: ' . $file);
    http_response_code(404);
    echo '404 - not found: ' . htmlspecialchars($path);
    exit;
}

// ---- Everything else: let PHP handle it normally ----------------------------
error_log('[router] pass-through: ' . $path);
return false;
