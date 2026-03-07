<?php
/**
 * agent-flow/api/list.php
 * GET -- returns {ok, flows: [name, ...]}
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

define('FLOWS_DIR', __DIR__ . '/../flows');

try {
    if (!is_dir(FLOWS_DIR)) {
        echo json_encode(['ok' => true, 'flows' => []]);
        exit;
    }

    $files = glob(FLOWS_DIR . '/*.json');
    if ($files === false) {
        error_log('agent-flow/list.php: glob failed on ' . FLOWS_DIR);
        http_response_code(500);
        echo json_encode(['ok' => false, 'error' => 'Could not list flows directory.']);
        exit;
    }

    $names = array_map(fn($f) => basename($f, '.json'), $files);
    sort($names);

    echo json_encode(['ok' => true, 'flows' => $names]);

} catch (Throwable $e) {
    error_log('agent-flow/list.php: ' . $e->getMessage());
    http_response_code(500);
    echo json_encode(['ok' => false, 'error' => 'Internal error: ' . $e->getMessage()]);
}
