<?php
/**
 * wasm_bridge.php
 * ---------------
 * CLI / HTTP bridge that calls the Python WasmRenderer and returns PNG.
 *
 * ── CLI mode ─────────────────────────────────────────────────────────────
 *   php wasm_bridge.php <html_b64> <css_b64> <width> <height>
 *   Outputs raw PNG bytes to stdout.  Suitable for:
 *     $png = shell_exec("php wasm_bridge.php $h $c 1280 900");
 *
 * ── HTTP mode ────────────────────────────────────────────────────────────
 *   POST /wasm_bridge.php
 *   Content-Type: application/json
 *   { "html": "...", "css": "...", "width": 1280, "height": 900 }
 *   -> 200  Content-Type: image/png   (raw PNG)
 *      or
 *   -> 200  { "ok": true, "png_b64": "...", "width":..., "height":..., "metrics":{} }
 *      (when Accept: application/json or ?format=json in query string)
 *
 * Requires:
 *   - Python 3 with modules/wasm_renderer.py in the path above this file.
 *   - render_core.wasm in dev-browser/src/  OR  librender_core.dylib/.so
 *
 * Security:
 *   - CLI: only callable by PHP process owner (no remote access).
 *   - HTTP: gated behind WASM_BRIDGE_SECRET env var (if set) checked via
 *           X-Bridge-Secret header or ?secret= query param.
 */

declare(strict_types=1);

// ---------------------------------------------------------------------------
// Helper: locate Python executable
// ---------------------------------------------------------------------------
function find_python(): string {
    foreach (['python3', 'python', '/usr/bin/python3', '/usr/local/bin/python3'] as $p) {
        $out = @shell_exec("command -v " . escapeshellcmd($p) . " 2>/dev/null");
        if ($out && trim($out) !== '') return trim($out);
    }
    throw new RuntimeException('Python 3 not found on PATH');
}

// ---------------------------------------------------------------------------
// Helper: locate this project's root (one level above wasm_bridge.php)
// ---------------------------------------------------------------------------
function project_root(): string {
    return dirname(__FILE__);
}

// ---------------------------------------------------------------------------
// Core: call Python wasm_renderer, return PNG bytes
// ---------------------------------------------------------------------------
function render_wasm(string $html, string $css, int $width, int $height): string {
    $py    = find_python();
    $root  = project_root();
    $mod   = $root . '/modules/wasm_renderer.py';

    if (!file_exists($mod)) {
        throw new RuntimeException("modules/wasm_renderer.py not found at $mod");
    }

    // Write html + css to temp files (avoid argv/shell-injection via large strings).
    $tmp_html = tempnam(sys_get_temp_dir(), 'xcm_html_');
    $tmp_css  = tempnam(sys_get_temp_dir(), 'xcm_css_');
    $tmp_png  = tempnam(sys_get_temp_dir(), 'xcm_out_') . '.png';

    file_put_contents($tmp_html, $html);
    file_put_contents($tmp_css,  $css);

    $cmd = sprintf(
        '%s %s %s %s %s %dx%d 2>&1',
        escapeshellcmd($py),
        escapeshellarg($mod),
        escapeshellarg($tmp_html),
        escapeshellarg($tmp_css),
        escapeshellarg($tmp_png),
        max(1, $width),
        max(1, $height)
    );

    $env = 'PYTHONPATH=' . escapeshellarg($root);
    $output = shell_exec("$env $cmd");

    @unlink($tmp_html);
    @unlink($tmp_css);

    if (!file_exists($tmp_png)) {
        @unlink($tmp_png);
        throw new RuntimeException("Render failed. Python output:\n$output");
    }

    $png = file_get_contents($tmp_png);
    @unlink($tmp_png);

    if ($png === false || strlen($png) < 8) {
        throw new RuntimeException("Invalid PNG output. Python output:\n$output");
    }

    return $png;
}

// ---------------------------------------------------------------------------
// Security check (HTTP mode)
// ---------------------------------------------------------------------------
function check_secret(): void {
    $secret = getenv('WASM_BRIDGE_SECRET');
    if (!$secret) return;   // no secret configured → open access

    $provided = $_SERVER['HTTP_X_BRIDGE_SECRET']
             ?? $_GET['secret']
             ?? '';

    if (!hash_equals($secret, $provided)) {
        http_response_code(403);
        header('Content-Type: application/json');
        echo json_encode(['ok' => false, 'error' => 'Forbidden']);
        exit;
    }
}

// ---------------------------------------------------------------------------
// CLI mode
// ---------------------------------------------------------------------------
if (PHP_SAPI === 'cli') {
    if (count($argv) < 5) {
        fwrite(STDERR, "Usage: php wasm_bridge.php <html_b64> <css_b64> <width> <height>\n");
        exit(1);
    }

    $html   = base64_decode($argv[1], true);
    $css    = base64_decode($argv[2], true);
    $width  = (int) $argv[3];
    $height = (int) $argv[4];

    if ($html === false) {
        fwrite(STDERR, "Error: html_b64 is not valid base64\n");
        exit(1);
    }

    try {
        $png = render_wasm($html, $css ?: '', $width, $height);
        fwrite(STDOUT, $png);
    } catch (Throwable $e) {
        fwrite(STDERR, 'Error: ' . $e->getMessage() . "\n");
        exit(1);
    }
    exit(0);
}

// ---------------------------------------------------------------------------
// HTTP mode
// ---------------------------------------------------------------------------
check_secret();

// Parse request body.
$content_type = $_SERVER['CONTENT_TYPE'] ?? '';
$want_json    = (
    str_contains($content_type, 'application/json')
    || (($_GET['format'] ?? '') === 'json')
    || str_contains($_SERVER['HTTP_ACCEPT'] ?? '', 'application/json')
);

$html   = '';
$css    = '';
$width  = 1280;
$height = 900;

if (str_contains($content_type, 'application/json')) {
    $body = file_get_contents('php://input');
    $data = json_decode($body, true);
    if (!is_array($data)) {
        http_response_code(400);
        header('Content-Type: application/json');
        echo json_encode(['ok' => false, 'error' => 'Invalid JSON body']);
        exit;
    }
    $html   = $data['html']   ?? '';
    $css    = $data['css']    ?? '';
    $width  = (int)($data['width']  ?? 1280);
    $height = (int)($data['height'] ?? 900);
} else {
    // Fall back to POST fields.
    $html   = $_POST['html']   ?? '';
    $css    = $_POST['css']    ?? '';
    $width  = (int)($_POST['width']  ?? 1280);
    $height = (int)($_POST['height'] ?? 900);
}

if ($html === '') {
    http_response_code(400);
    header('Content-Type: application/json');
    echo json_encode(['ok' => false, 'error' => 'html is required']);
    exit;
}

// Render.
try {
    $png = render_wasm($html, $css, max(1, $width), max(1, $height));
} catch (Throwable $e) {
    http_response_code(500);
    header('Content-Type: application/json');
    echo json_encode(['ok' => false, 'error' => $e->getMessage()]);
    exit;
}

if ($want_json) {
    header('Content-Type: application/json');
    echo json_encode([
        'ok'       => true,
        'png_b64'  => base64_encode($png),
        'width'    => $width,
        'height'   => $height,
    ]);
} else {
    header('Content-Type: image/png');
    header('Content-Length: ' . strlen($png));
    header('Cache-Control: no-store');
    echo $png;
}
