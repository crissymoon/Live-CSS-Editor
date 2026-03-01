<?php
/**
 * agent-flow/api/run.php
 * Accepts POST JSON: {moon: string, flow: string}
 * Writes moon source to a temp file, runs it with the moon binary,
 * returns JSON: {ok, stdout, stderr, exit_code, moon_src, error?}
 *
 * Fallback error handling: every failure path logs to error_log and
 * returns a structured JSON error body so the browser always gets context.
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

// Timeout for the moon process in seconds.
define('RUN_TIMEOUT', 30);

// Candidate paths for the moon binary.
$MOON_CANDIDATES = [
    '/Users/mac/Desktop/xcm-editor/moon-lang/moon',
    '/usr/local/bin/moon',
    'moon',
];

function findMoon(array $candidates): ?string {
    foreach ($candidates as $path) {
        if (strpos($path, '/') === 0) {
            if (is_executable($path)) return $path;
        } else {
            $out = shell_exec('which ' . escapeshellarg($path) . ' 2>/dev/null');
            if ($out && is_executable(trim($out))) return trim($out);
        }
    }
    return null;
}

function jsonErr(string $msg, int $http = 500): void {
    http_response_code($http);
    echo json_encode(['ok' => false, 'error' => $msg, 'stdout' => '', 'stderr' => $msg, 'exit_code' => -1]);
    exit;
}

// Only accept POST.
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    jsonErr('Method not allowed', 405);
}

// Read and decode body.
$raw = file_get_contents('php://input');
if (!$raw) {
    jsonErr('Empty request body', 400);
}

$body = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE) {
    jsonErr('Invalid JSON body: ' . json_last_error_msg(), 400);
}

$moonSrc = $body['moon'] ?? '';
if (!$moonSrc || !trim($moonSrc)) {
    jsonErr('No moon source provided', 400);
}

// Locate moon binary.
$moonBin = findMoon($MOON_CANDIDATES);
if (!$moonBin) {
    error_log('agent-flow/run.php: moon binary not found');
    jsonErr('moon binary not found. Check /Users/mac/Desktop/xcm-editor/moon-lang/ or PATH.', 503);
}

// Write moon source to a temp file.
$tmpFile = tempnam(sys_get_temp_dir(), 'agentflow_') . '.moon';
if (!file_put_contents($tmpFile, $moonSrc)) {
    error_log('agent-flow/run.php: could not write temp file ' . $tmpFile);
    jsonErr('Could not write temporary moon script.', 500);
}

// Run the moon binary with the script.
try {
    $desc = [
        0 => ['pipe', 'r'],
        1 => ['pipe', 'w'],
        2 => ['pipe', 'w'],
    ];
    $cmd  = escapeshellarg($moonBin) . ' ' . escapeshellarg($tmpFile);
    $proc = proc_open($cmd, $desc, $pipes, sys_get_temp_dir());

    if (!is_resource($proc)) {
        @unlink($tmpFile);
        error_log('agent-flow/run.php: proc_open failed for: ' . $cmd);
        jsonErr('proc_open failed -- cannot run moon.', 500);
    }

    fclose($pipes[0]);

    // Set non-blocking so we can apply a timeout.
    stream_set_blocking($pipes[1], false);
    stream_set_blocking($pipes[2], false);

    $stdout = '';
    $stderr = '';
    $start  = time();
    $done   = false;

    while (!$done) {
        $status = proc_get_status($proc);
        if (!$status['running']) {
            $done = true;
        } elseif (time() - $start > RUN_TIMEOUT) {
            proc_terminate($proc, 9);
            $stderr .= "\n[agent-flow] Process killed after " . RUN_TIMEOUT . "s timeout.";
            $done = true;
        }
        $stdout .= stream_get_contents($pipes[1]);
        $stderr .= stream_get_contents($pipes[2]);
        if (!$done) usleep(50000);
    }

    // Drain any remaining output.
    $stdout .= stream_get_contents($pipes[1]);
    $stderr .= stream_get_contents($pipes[2]);
    fclose($pipes[1]);
    fclose($pipes[2]);
    $exitCode = proc_close($proc);

    if ($stderr) {
        error_log('agent-flow/run.php moon stderr: ' . substr($stderr, 0, 500));
    }

    @unlink($tmpFile);

    echo json_encode([
        'ok'        => true,
        'stdout'    => $stdout,
        'stderr'    => $stderr,
        'exit_code' => $exitCode,
        'moon_src'  => $moonSrc,
    ]);

} catch (Throwable $e) {
    @unlink($tmpFile);
    error_log('agent-flow/run.php: ' . $e->getMessage());
    jsonErr('Run exception: ' . $e->getMessage(), 500);
}
