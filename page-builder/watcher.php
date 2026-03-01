<?php
/**
 * page-builder/watcher.php
 *
 * Serves a built page with the live DOM editor injected.
 * Reads pages/{page}/index.html, injects pb-editor.js and pb-editor.css,
 * and serves the result in the browser.
 *
 * Usage: watcher.php?page=demo
 * Options:
 *   &rebuild=1   -- rebuild from JSON before serving
 *   &reset=1     -- delete overrides.json and rebuild
 */

declare(strict_types=1);

$page = $_GET['page'] ?? '';

if (!$page || !preg_match('/^[a-z0-9_-]+$/i', $page)) {
    http_response_code(400);
    echo '<pre>Missing or invalid ?page= parameter. Example: watcher.php?page=demo</pre>';
    exit;
}

$pageDir  = __DIR__ . '/pages/' . $page;
$htmlFile = $pageDir . '/index.html';

if (!is_dir($pageDir)) {
    http_response_code(404);
    echo '<pre>Page directory not found: ' . htmlspecialchars($pageDir) . '</pre>';
    exit;
}

// Handle reset - clears all saved overrides
if (isset($_GET['reset'])) {
    $of = $pageDir . '/overrides.json';
    if (file_exists($of)) {
        if (!unlink($of)) {
            error_log('[watcher] Could not delete overrides: ' . $of);
        }
    }
    // Redirect without reset param to avoid re-triggering on refresh
    $url = '?page=' . urlencode($page) . '&rebuild=1';
    header('Location: ' . $url);
    exit;
}

// Rebuild from JSON if requested or if index.html doesn't exist yet
$needBuild = isset($_GET['rebuild']) || !file_exists($htmlFile);
if ($needBuild) {
    $buildScript = __DIR__ . '/build.php';
    if (!file_exists($buildScript)) {
        http_response_code(500);
        echo '<pre>build.php not found at: ' . htmlspecialchars($buildScript) . '</pre>';
        exit;
    }
    // Run build.php in the same process
    $_GET_backup  = $_GET;
    $_GET['page'] = $page;
    ob_start();
    try {
        require $buildScript;
    } catch (Throwable $e) {
        ob_end_clean();
        http_response_code(500);
        echo '<pre>Build error: ' . htmlspecialchars($e->getMessage()) . "\n"
            . htmlspecialchars($e->getFile()) . ':' . $e->getLine() . '</pre>';
        exit;
    }
    ob_end_clean();
    $_GET = $_GET_backup;

    if (!file_exists($htmlFile)) {
        http_response_code(500);
        echo '<pre>Build completed but index.html was not created. Check server error logs.</pre>';
        exit;
    }
}

// Read the built HTML
$html = file_get_contents($htmlFile);
if ($html === false) {
    http_response_code(500);
    echo '<pre>Cannot read: ' . htmlspecialchars($htmlFile) . '</pre>';
    exit;
}

// Determine base URL for editor assets (relative to this file)
$scriptBase = rtrim(dirname($_SERVER['SCRIPT_NAME'] ?? '/page-builder/watcher.php'), '/');

// Build the editor toolbar HTML (injected at the top of <body>)
$toolbar = <<<HTML
<div id="pb-toolbar" style="
    position:fixed;top:0;left:0;right:0;z-index:9999;
    background:#0d0d18;border-bottom:1px solid rgba(99,102,241,0.35);
    display:flex;align-items:center;gap:10px;padding:0 16px;height:42px;
    font-family:'JetBrains Mono','Fira Code',monospace;font-size:11px;">
  <span style="color:#6366f1;font-weight:bold;letter-spacing:0.1em;text-transform:uppercase;">Page Builder</span>
  <span style="color:#444;margin:0 2px;">|</span>
  <span id="pb-page-name" style="color:#8888a0;letter-spacing:0.06em;">{$page}</span>
  <span style="color:#444;margin:0 2px;">|</span>
  <span id="pb-edit-status" style="color:#10b981;font-size:10px;">right-click any element to edit</span>
  <div style="flex:1;"></div>
  <span id="pb-breakpoint" data-bp="xl" title="Current viewport breakpoint">xl</span>
  <button onclick="PBEditor.rebuild()" style="background:#1a1a2e;color:#8888a0;border:1px solid rgba(255,255,255,0.08);padding:4px 12px;cursor:pointer;font-family:inherit;font-size:10px;letter-spacing:0.06em;">Rebuild</button>
  <button onclick="PBEditor.reset()" style="background:#1a1a2e;color:#ef4444;border:1px solid rgba(239,68,68,0.3);padding:4px 12px;cursor:pointer;font-family:inherit;font-size:10px;letter-spacing:0.06em;">Reset</button>
  <a href="index.php" style="background:#1a1a2e;color:#8888a0;border:1px solid rgba(255,255,255,0.08);padding:4px 12px;text-decoration:none;font-size:10px;letter-spacing:0.06em;">Back</a>
</div>
<div style="height:42px;"></div>
HTML;

// Inject assets before </head>
$editorCssTag    = '<link rel="stylesheet" href="' . $scriptBase . '/css/pb-editor.css">';
$responsiveCssTag = '<link rel="stylesheet" href="' . $scriptBase . '/css/pb-responsive.css">';
// Push sticky header below the fixed watcher toolbar (42px) so the mobile
// nav menu does not open behind or under the toolbar text.
$watcherOffsetCss = '<style>
/* Watcher: push sticky header below the fixed toolbar */
header[data-pb-nav-collapse] { top: 42px !important; }
</style>';
$html = str_replace('</head>', $editorCssTag . "\n" . $responsiveCssTag . "\n" . $watcherOffsetCss . "\n</head>", $html);

// Inject toolbar and editor + responsive scripts before </body>
$editorJsTag    = '<script src="' . $scriptBase . '/js/pb-editor.js"></script>';
$responsiveJsTag = '<script src="' . $scriptBase . '/js/pb-responsive.js"></script>';
$pbConfig    = '<script>window.PB_CONFIG = ' . json_encode([
    'page'    => $page,
    'saveUrl' => $scriptBase . '/save.php',
]) . ';</script>';

$html = str_replace('<body>', '<body>' . "\n" . $toolbar, $html);
$html = str_replace('</body>', $pbConfig . "\n" . $editorJsTag . "\n" . $responsiveJsTag . "\n</body>", $html);

header('Content-Type: text/html; charset=UTF-8');
header('Cache-Control: no-store');
echo $html;
