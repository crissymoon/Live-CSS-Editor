<?php
/**
 * agent-flow/api/validate.php
 * Returns JSON indicating whether the moon binary is available and working.
 * Always returns JSON; errors are included in the payload so the browser
 * has something to display rather than an opaque HTTP error.
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

// Candidate paths for the moon binary, checked in order.
$CANDIDATES = [
    '/Users/mac/Desktop/xcm-editor/moon-lang/moon',
    '/usr/local/bin/moon',
    'moon',  // on PATH
];

function findMoon(array $candidates): ?string {
    foreach ($candidates as $path) {
        if (strpos($path, '/') === 0) {
            if (is_executable($path)) return $path;
        } else {
            // bare name -- check PATH via which
            $out = shell_exec('which ' . escapeshellarg($path) . ' 2>/dev/null');
            if ($out && is_executable(trim($out))) return trim($out);
        }
    }
    return null;
}

try {
    $bin = findMoon($CANDIDATES);
    if (!$bin) {
        echo json_encode([
            'ok'    => false,
            'error' => 'moon binary not found. Checked: ' . implode(', ', $CANDIDATES),
        ]);
        exit;
    }

    // Run "moon --version" (or "moon" with no args) to get version string.
    $desc = [
        0 => ['pipe','r'],
        1 => ['pipe','w'],
        2 => ['pipe','w'],
    ];
    $proc = proc_open(
        escapeshellarg($bin) . ' --version 2>&1',
        $desc, $pipes
    );
    if (!is_resource($proc)) {
        echo json_encode(['ok' => false, 'error' => 'proc_open failed for moon --version']);
        exit;
    }
    fclose($pipes[0]);
    $out = stream_get_contents($pipes[1]);
    fclose($pipes[1]);
    fclose($pipes[2]);
    $exit = proc_close($proc);

    // moon may not support --version; as long as it ran, that is fine.
    echo json_encode([
        'ok'      => true,
        'bin'     => $bin,
        'version' => trim($out) ?: 'available',
        'exit'    => $exit,
    ]);

} catch (Throwable $e) {
    error_log('agent-flow/validate.php: ' . $e->getMessage());
    echo json_encode(['ok' => false, 'error' => 'Internal error: ' . $e->getMessage()]);
}
