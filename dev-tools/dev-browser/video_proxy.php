<?php
/**
 * video_proxy.php -- HTTPS pass-through proxy for video resources.
 *
 * Upload to your HTTPS server.  The browser loads video through this
 * endpoint to avoid mixed-content blocks (HTTPS page -> HTTP resource)
 * and CORS restrictions.
 *
 * Usage:
 *   https://yourdomain.com/video_proxy.php?url=https%3A%2F%2Fcdn.example.com%2Fvideo.mp4
 *
 * Security: only allows requests to known CDN/video domains.
 */

// -- CORS headers ----------------------------------------------------------
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, OPTIONS");
header("Access-Control-Allow-Headers: Range, Authorization, Content-Type");
header("Access-Control-Expose-Headers: Content-Length, Content-Range, Content-Type");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit(0);
}

// -- Target URL -------------------------------------------------------------
$url = $_GET['url'] ?? '';
if (!$url) {
    http_response_code(400);
    die("Error: No URL provided.");
}

// Resolve encoded ".." path segments that some DASH manifests produce.
// e.g. /mpds/../video/  ->  /video/
$url = preg_replace('#/[^/]+/\.\./#', '/', $url);

// -- Domain whitelist -------------------------------------------------------
// Only proxy requests to known video CDN domains.  Add more as needed.
$allowed_domains = [
    'cdn.bitmovin.com',
    'bitmovin.com',
    'bitdash-a.akamaihd.net',
    'dms.licdn.com',
    'shaka-player-demo.appspot.com',
    'storage.googleapis.com',
    'demo.unified-streaming.com',
    'cdn.jwplayer.com',
    'cdn.flowplayer.com',
    'vod.akamaized.net',
    'dash.akamaized.net',
    'linear.akamaized.net',
    'media.axprod.net',
    'bitcdn.org',
    'f.vimeocdn.com',
    'player.vimeo.com',
    'cdn.vidible.tv',
    'v.redd.it',
    'video.twimg.com',
    'video.xx.fbcdn.net',
];

$parsed = parse_url($url);
$host = $parsed['host'] ?? '';
$domain_ok = false;
foreach ($allowed_domains as $d) {
    if ($host === $d || str_ends_with($host, '.' . $d)) {
        $domain_ok = true;
        break;
    }
}
if (!$domain_ok) {
    http_response_code(403);
    die("Error: Domain not allowed: " . htmlspecialchars($host));
}

// -- Fetch upstream ---------------------------------------------------------
$req_headers = [
    "User-Agent: " . ($_SERVER['HTTP_USER_AGENT'] ?? 'Mozilla/5.0')
];

// Forward Range header (critical for video seeking).
$all_headers = function_exists('getallheaders') ? getallheaders() : [];
if (isset($all_headers['Range'])) {
    $req_headers[] = "Range: " . $all_headers['Range'];
}

$ctx = stream_context_create([
    "http" => [
        "method"          => "GET",
        "follow_location" => 1,
        "max_redirects"   => 5,
        "ignore_errors"   => true,
        "timeout"         => 30,
        "header"          => $req_headers,
    ],
    "ssl" => [
        "verify_peer"      => true,
        "verify_peer_name" => true,
    ],
]);

$upstream = @fopen($url, 'rb', false, $ctx);
if (!$upstream) {
    http_response_code(502);
    die("Error: Upstream unreachable for " . htmlspecialchars($url));
}

// -- Forward upstream response headers --------------------------------------
$meta = stream_get_meta_data($upstream);
$skip = ['transfer-encoding', 'connection', 'keep-alive', 'access-control'];
foreach ($meta['wrapper_data'] as $hdr) {
    $lower = strtolower($hdr);
    $dominated = false;
    foreach ($skip as $s) {
        if (strpos($lower, $s) !== false) { $dominated = true; break; }
    }
    if (!$dominated) {
        header($hdr);
    }
}

// -- Stream to client -------------------------------------------------------
$out = fopen("php://output", "wb");
stream_copy_to_stream($upstream, $out);
fclose($upstream);
fclose($out);
