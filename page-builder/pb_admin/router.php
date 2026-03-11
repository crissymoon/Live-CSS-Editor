<?php
/**
 * pb_admin/router.php
 * Router script for PHP's built-in dev server.
 *
 * Usage (from repo root):
 *   php -S localhost:8080 pb_admin/router.php
 *
 * Rules:
 *  - Requests under /pb_admin/ or /page-builder/pb_admin/ are served normally.
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
        $prefix = (strpos($path, '/page-builder/') === 0) ? '/page-builder/pb_admin/' : '/pb_admin/';
        $dest = $prefix . $f . $qs;
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
if ($path === '/page-builder/pb_admin') {
    error_log('[router] /page-builder/pb_admin -> redirect /page-builder/pb_admin/');
    header('Location: /page-builder/pb_admin/', true, 301);
    exit;
}

// ---- Route /pb_admin/* (and /page-builder/pb_admin/*) to the correct file ---
// PHP 8.4 built-in server: "return false" can misbehave when the router lives
// in a subdirectory of the document root. We resolve and require the file
// directly so PHP parses and executes it with all variables ($_GET, $_POST,
// $_SERVER['REQUEST_URI'], etc.) intact.
if (strpos($path, '/pb_admin/') === 0 || strpos($path, '/page-builder/pb_admin/') === 0) {
    $cleanPath = strtok($path, '?');
    if (strpos($cleanPath, '/page-builder/pb_admin/') === 0) {
        $cleanPath = substr($cleanPath, strlen('/page-builder'));
    }
    $file = realpath(__DIR__ . '/../' . ltrim($cleanPath, '/'));
    if ($file !== false && is_file($file)) {
        error_log('[router] admin route -> require ' . $file);
        // Set SCRIPT_FILENAME so any code that inspects it gets the right value.
        $_SERVER['SCRIPT_FILENAME'] = $file;
        require $file;
        exit;
    }
    // File not found inside pb_admin -- return 404.
    error_log('[router] admin route file not found: ' . $path);
    http_response_code(404);
    echo '404 - not found: ' . htmlspecialchars($path);
    exit;
}

// ---- Everything else: let PHP handle it normally ----------------------------
error_log('[router] pass-through: ' . $path);
return false;
