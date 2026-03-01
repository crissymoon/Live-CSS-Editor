<?php
/**
 * page-builder/stage.php
 *
 * Staging engine: builds all non-blocked pages, minifies the HTML output,
 * writes everything to  page-builder/deploy/  and records analytics in
 *  deploy/build-log.json.
 *
 * GET  stage.php            -> returns { "ok": true, "token": "...", "pages": [...] }
 * POST stage.php            -> runs the full deploy, requires { "token": "..." }
 *
 * Security:
 *   A random per-install token is generated once and stored in .stage-token.
 *   The token must be supplied in every POST as  Authorization: Bearer <token>
 *   or in the JSON body as { "token": "..." }.
 *   GETs return the token so the UI can read it (local-only tool).
 *
 * Each page that has  pages/{name}/.flags.json  with  "blocked": true  is
 * skipped. All other valid pages are built and staged.
 *
 * Deploy layout:
 *   deploy/
 *     index.html          <- auto-generated nav page for the staged project
 *     build-log.json      <- analytics (read by the admin dashboard widget)
 *     {pagename}/
 *       index.html        <- minified built page
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, Authorization');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

// ---- Include build functions (skips main execution via constant guard) ------
define('PB_STAGE_INCLUDE', 1);
require_once __DIR__ . '/build.php';

// ---- Helpers ----------------------------------------------------------------

function stageErr(string $msg, int $code = 400): void {
    error_log('[stage] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function stageOk(array $data): void {
    echo json_encode(array_merge(['ok' => true], $data), JSON_PRETTY_PRINT);
    exit;
}

// ---- Token management -------------------------------------------------------

$tokenFile = __DIR__ . '/.stage-token';

function getOrCreateToken(string $tokenFile): string {
    if (file_exists($tokenFile)) {
        $t = trim((string) file_get_contents($tokenFile));
        if (strlen($t) >= 32) {
            return $t;
        }
        error_log('[stage] .stage-token was too short, regenerating');
    }
    try {
        $t = bin2hex(random_bytes(24));
    } catch (Throwable $e) {
        error_log('[stage] random_bytes failed: ' . $e->getMessage() . ' - falling back to uniqid');
        $t = hash('sha256', uniqid('stage', true) . microtime(true));
    }
    if (file_put_contents($tokenFile, $t) === false) {
        error_log('[stage] WARNING: could not write .stage-token to ' . $tokenFile);
    }
    return $t;
}

function validateToken(string $expected): void {
    // Try Authorization header first
    $header = '';
    if (function_exists('getallheaders')) {
        $h = getallheaders();
        $header = $h['Authorization'] ?? $h['authorization'] ?? '';
    }
    if (str_starts_with($header, 'Bearer ')) {
        $supplied = substr($header, 7);
        if (hash_equals($expected, $supplied)) return;
        stageErr('Invalid token in Authorization header', 403);
    }

    // Try JSON body
    $raw  = file_get_contents('php://input');
    $body = json_decode($raw ?: '{}', true);
    if (!is_array($body)) {
        stageErr('Invalid JSON body', 400);
    }
    $supplied = $body['token'] ?? '';
    if (!$supplied) {
        stageErr('Missing token. Provide it in Authorization: Bearer <token> header or in POST body as {"token":"..."}', 403);
    }
    if (!hash_equals($expected, (string)$supplied)) {
        stageErr('Invalid token', 403);
    }
}

// ---- Read page flags --------------------------------------------------------

function readPageFlags(string $pageDir): array {
    $file = $pageDir . '/.flags.json';
    if (!file_exists($file)) return [];
    $raw = file_get_contents($file);
    if ($raw === false) {
        error_log('[stage] could not read .flags.json in: ' . $pageDir);
        return [];
    }
    $data = json_decode($raw, true);
    return is_array($data) ? $data : [];
}

// ---- Scan pages directory ---------------------------------------------------

function scanPages(string $pagesRoot): array {
    $pages = [];
    if (!is_dir($pagesRoot)) {
        return $pages;
    }
    foreach (scandir($pagesRoot) as $name) {
        if ($name === '.' || $name === '..') continue;
        $dir = $pagesRoot . '/' . $name;
        if (!is_dir($dir)) continue;
        if (!preg_match('/^[a-z0-9_-]+$/i', $name)) continue;

        $flags   = readPageFlags($dir);
        $blocked = !empty($flags['blocked']);
        $built   = file_exists($dir . '/index.html')
            ? filemtime($dir . '/index.html')
            : null;

        $pages[] = [
            'name'    => $name,
            'dir'     => $dir,
            'blocked' => $blocked,
            'built'   => $built ? date('c', $built) : null,
        ];
    }
    return $pages;
}

// ---- Write deploy index (navigation) ----------------------------------------

function writeDeployIndex(string $deployDir, array $staged): void {
    $links = '';
    foreach ($staged as $p) {
        $url   = './' . rawurlencode($p['name']) . '/';
        $name  = htmlspecialchars($p['name'], ENT_QUOTES);
        $bytes = number_format($p['bytes_min'] ?? 0);
        $links .= '<li><a href="' . $url . '">' . $name . '</a><span class="sz"> ' . $bytes . ' B</span></li>';
    }
    $count = count($staged);
    $ts    = date('Y-m-d H:i:s');

    $html = <<<HTML
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Staged Deploy</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#0d0d18;color:#a0a0c0;font-family:'JetBrains Mono',monospace;padding:40px 24px;}
h1{color:#6366f1;font-size:16px;letter-spacing:.1em;margin-bottom:8px;}
p.meta{font-size:11px;color:#444470;margin-bottom:32px;}
ul{list-style:none;display:flex;flex-direction:column;gap:8px;}
li{display:flex;align-items:center;gap:12px;}
a{color:#8888c0;font-size:13px;text-decoration:none;padding:8px 14px;border:1px solid rgba(99,102,241,.2);display:block;}
a:hover{background:rgba(99,102,241,.1);color:#c0c0f0;border-color:rgba(99,102,241,.5);}
.sz{font-size:10px;color:#333360;}
</style>
</head>
<body>
<h1>&gt; staged deploy</h1>
<p class="meta">{$count} page(s) built &mdash; {$ts}</p>
<ul>{$links}</ul>
</body>
</html>
HTML;

    if (file_put_contents($deployDir . '/index.html', $html) === false) {
        error_log('[stage] WARNING: could not write deploy/index.html');
    }
}

// ===========================================================================
// REQUEST ROUTING
// ===========================================================================

$pagesRoot = __DIR__ . '/pages';
$deployDir = __DIR__ . '/deploy';
$logFile   = $deployDir . '/build-log.json';
$token     = getOrCreateToken($tokenFile);

// ---- GET: return token + page summary for the UI --------------------------

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    $pages = scanPages($pagesRoot);
    $summary = array_map(function ($p) {
        return ['name' => $p['name'], 'blocked' => $p['blocked'], 'built' => $p['built']];
    }, $pages);
    stageOk(['token' => $token, 'pages' => $summary]);
}

// ---- POST: run staging ----------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    stageErr('GET or POST required', 405);
}

validateToken($token);

// Load project config (index page + slug map)
$projectJson = __DIR__ . '/project.json';
$projectCfg  = [];
if (file_exists($projectJson)) {
    $raw = file_get_contents($projectJson);
    if ($raw !== false) {
        $decoded = json_decode($raw, true);
        if (is_array($decoded)) $projectCfg = $decoded;
    }
}
$slugMap   = is_array($projectCfg['slugs']    ?? null) ? $projectCfg['slugs']    : [];
$indexPage = is_string($projectCfg['index_page'] ?? null) ? $projectCfg['index_page'] : '';
error_log('[stage] project config loaded -- index_page="' . $indexPage . '", slugs=' . count($slugMap));

$startTime = microtime(true);
$pages     = scanPages($pagesRoot);
$staged    = [];
$skipped   = [];
$errors    = [];

// Ensure deploy directory exists
if (!is_dir($deployDir)) {
    if (!mkdir($deployDir, 0755, true)) {
        stageErr('Cannot create deploy directory: ' . $deployDir, 500);
    }
}

foreach ($pages as $p) {
    $name = $p['name'];

    if ($p['blocked']) {
        $skipped[] = $name;
        error_log('[stage] skipping blocked page: ' . $name);
        continue;
    }

    try {
        // Build page HTML using shared function from build.php
        $data    = buildPageData($name, $pagesRoot, $slugMap);
        $raw     = $data['html'];
        $minHtml = minifyHtml($raw);

        // Write to deploy directory
        $destDir = $deployDir . '/' . $name;
        if (!is_dir($destDir)) {
            if (!mkdir($destDir, 0755, true)) {
                throw new RuntimeException('Cannot create deploy dir: ' . $destDir);
            }
        }
        $destFile = $destDir . '/index.html';
        if (file_put_contents($destFile, $minHtml) === false) {
            throw new RuntimeException('Cannot write: ' . $destFile);
        }

        $bytesRaw = $data['bytes_raw'];
        $bytesMin = strlen($minHtml);
        $saved    = $bytesRaw > 0 ? round((1 - $bytesMin / $bytesRaw) * 100, 1) : 0;

        $staged[] = [
            'name'       => $name,
            'title'      => $data['title'],
            'bytes_raw'  => $bytesRaw,
            'bytes_min'  => $bytesMin,
            'saved_pct'  => $saved,
            'staged_at'  => date('c'),
            'url'        => '/page-builder/deploy/' . $name . '/',
        ];

        error_log('[stage] staged page: ' . $name . ' (' . $bytesRaw . 'B raw -> ' . $bytesMin . 'B min, ' . $saved . '% saved)');

    } catch (Throwable $e) {
        $msg = 'Build failed for "' . $name . '": ' . $e->getMessage();
        $errors[] = $msg;
        error_log('[stage] ERROR: ' . $msg);
    }
}

// If a specific page is designated as the index, copy its minified HTML to
// deploy/index.html so it serves as the root entry point.
$indexWritten = false;
if ($indexPage !== '') {
    $indexSrc = $deployDir . '/' . $indexPage . '/index.html';
    if (file_exists($indexSrc)) {
        $indexHtml = file_get_contents($indexSrc);
        if ($indexHtml !== false && file_put_contents($deployDir . '/index.html', $indexHtml) !== false) {
            $indexWritten = true;
            error_log('[stage] wrote deploy/index.html from page "' . $indexPage . '"');
        } else {
            error_log('[stage] WARNING: could not write deploy/index.html from "' . $indexPage . '"');
        }
    } else {
        error_log('[stage] WARNING: index page "' . $indexPage . '" was not staged (blocked or error?)');
    }
}

// Fall back to navigation page only when no specific index page was configured
if (!$indexWritten) {
    writeDeployIndex($deployDir, $staged);
}

$elapsed     = round(microtime(true) - $startTime, 3);
$totalRaw    = array_sum(array_column($staged, 'bytes_raw'));
$totalMin    = array_sum(array_column($staged, 'bytes_min'));
$savedTotal  = $totalRaw > 0 ? round((1 - $totalMin / $totalRaw) * 100, 1) : 0;

// Analytics log
$log = [
    'last_deploy'        => date('c'),
    'elapsed_sec'        => $elapsed,
    'pages_staged'       => count($staged),
    'pages_skipped'      => count($skipped),
    'pages_errored'      => count($errors),
    'total_bytes_raw'    => $totalRaw,
    'total_bytes_min'    => $totalMin,
    'total_saved_pct'    => $savedTotal,
    'staged_pages'       => $staged,
    'skipped_pages'      => $skipped,
    'errors'             => $errors,
    'deploy_url'         => '/page-builder/deploy/',
];

if (file_put_contents($logFile, json_encode($log, JSON_PRETTY_PRINT)) === false) {
    error_log('[stage] WARNING: could not write build-log.json to: ' . $logFile);
}

error_log('[stage] deploy complete: ' . count($staged) . ' staged, ' . count($skipped) . ' skipped, ' . count($errors) . ' errors, ' . $elapsed . 's');

stageOk([
    'pages_staged'    => count($staged),
    'pages_skipped'   => count($skipped),
    'pages_errored'   => count($errors),
    'total_bytes_raw' => $totalRaw,
    'total_bytes_min' => $totalMin,
    'total_saved_pct' => $savedTotal,
    'elapsed_sec'     => $elapsed,
    'staged'          => $staged,
    'skipped'         => $skipped,
    'errors'          => $errors,
    'deploy_url'      => '/page-builder/deploy/',
]);
