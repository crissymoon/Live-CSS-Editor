<?php
/**
 * view.php -- Shared design viewer.
 *
 * Called by server.js when a /view/{token} URL is hit.
 * Looks up the token in shared_links, fetches the project HTML/CSS/JS,
 * and returns a self-contained HTML page that renders the design.
 *
 * GET ?token={8-char hex token}
 */

declare(strict_types=1);

define('DB_PATH',  __DIR__ . '/../data/projects.db');

function getDb(): SQLite3 {
    $db = new SQLite3(DB_PATH);
    $db->busyTimeout(2000);
    $db->exec('PRAGMA journal_mode=WAL');
    return $db;
}

function fail(string $msg, int $code = 404): void {
    http_response_code($code);
    header('Content-Type: text/html; charset=utf-8');
    echo '<!doctype html><html><head><meta charset="utf-8"><title>Not found</title>'
        . '<style>body{background:#111;color:#888;font-family:monospace;display:flex;align-items:center;justify-content:center;height:100vh;margin:0}'
        . 'p{font-size:14px;letter-spacing:.05em}</style></head>'
        . '<body><p>' . htmlspecialchars($msg) . '</p></body></html>';
    exit;
}

$token = $_GET['token'] ?? '';
if (!preg_match('/^[a-f0-9]{8}$/i', $token)) {
    fail('Invalid share link.');
}

if (!file_exists(DB_PATH)) {
    fail('No projects database found.');
}

$db   = getDb();
$stmt = $db->prepare(
    'SELECT sl.name, sl.views, p.html, p.css, p.js, p.updated_at
     FROM shared_links sl
     JOIN projects p ON p.name = sl.name
     WHERE sl.token = :token'
);
$stmt->bindValue(':token', $token, SQLITE3_TEXT);
$row = $stmt->execute()->fetchArray(SQLITE3_ASSOC);

if (!$row) {
    $db->close();
    fail('Share link not found or expired.');
}

// Increment view count
$upd = $db->prepare('UPDATE shared_links SET views = views + 1 WHERE token = :token');
$upd->bindValue(':token', $token, SQLITE3_TEXT);
$upd->execute();
$db->close();

$name       = htmlspecialchars($row['name']);
$updatedAt  = htmlspecialchars($row['updated_at']);
$html       = $row['html'] ?? '';
$css        = $row['css']  ?? '';
$js         = $row['js']   ?? '';

// Build the srcdoc for the preview iframe -- inline everything
$srcdoc = '<!doctype html><html><head>'
    . '<meta charset="utf-8">'
    . '<meta name="viewport" content="width=device-width,initial-scale=1">'
    . '<style>' . $css . '</style>'
    . '</head><body>'
    . $html
    . '<script>' . $js . '</script>'
    . '</body></html>';

// JSON-encode srcdoc for safe insertion into the JS below
$srcdocJson = json_encode($srcdoc, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);

header('Content-Type: text/html; charset=utf-8');
header('Cache-Control: no-store');
?>
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title><?= $name ?> -- Live CSS Share</title>
<style>
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

body {
    background: #111;
    color: #d4d4d4;
    font-family: 'Segoe UI', system-ui, sans-serif;
    display: flex;
    flex-direction: column;
    height: 100dvh;
    overflow: hidden;
}

.lcs-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 10px 16px;
    background: #1a1a1a;
    border-bottom: 1px solid #2a2a2a;
    flex-shrink: 0;
    gap: 12px;
}

.lcs-title {
    font-size: 13px;
    font-weight: 600;
    color: #d4d4d4;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.lcs-meta {
    font-size: 11px;
    color: #555;
    white-space: nowrap;
    flex-shrink: 0;
}

.lcs-badge {
    font-size: 10px;
    letter-spacing: .08em;
    color: #6a9fb5;
    text-transform: uppercase;
    flex-shrink: 0;
}

.lcs-frame {
    flex: 1;
    width: 100%;
    border: none;
    background: #fff;
    display: block;
}
</style>
</head>
<body>

<div class="lcs-header">
    <span class="lcs-badge">Live CSS</span>
    <span class="lcs-title"><?= $name ?></span>
    <span class="lcs-meta"><?= $updatedAt ?></span>
</div>

<iframe id="preview" class="lcs-frame" sandbox="allow-scripts" title="Design preview"></iframe>

<script>
(function () {
    var srcdoc = <?= $srcdocJson ?>;
    var frame = document.getElementById('preview');
    // Use srcdoc for sandboxed rendering -- no external requests, no cookies
    frame.srcdoc = srcdoc;
}());
</script>

</body>
</html>
