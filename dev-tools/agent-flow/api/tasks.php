<?php
/**
 * agent-flow/api/tasks.php
 *
 * Returns allowlisted task metadata from root smoke-tools.json for agent-task nodes.
 */

header('Content-Type: application/json');
header('X-Content-Type-Options: nosniff');

function jsonErr(string $msg, int $http = 500): void {
    http_response_code($http);
    echo json_encode(['ok' => false, 'error' => $msg, 'tasks' => []]);
    exit;
}

$root = realpath(__DIR__ . '/../../..');
if (!$root) {
    jsonErr('Could not resolve repository root');
}

$manifestPath = $root . '/smoke-tools.json';
if (!is_file($manifestPath)) {
    jsonErr('smoke-tools.json not found at repository root', 404);
}

$raw = file_get_contents($manifestPath);
if ($raw === false) {
    jsonErr('Could not read smoke-tools.json');
}

$data = json_decode($raw, true);
if (json_last_error() !== JSON_ERROR_NONE || !is_array($data)) {
    jsonErr('Invalid smoke-tools.json: ' . json_last_error_msg());
}

$tools = $data['tools'] ?? null;
if (!is_array($tools)) {
    jsonErr('smoke-tools.json missing tools array');
}

$tasks = [];
foreach ($tools as $tool) {
    if (!is_array($tool)) continue;
    $id = (string)($tool['id'] ?? '');
    if ($id === '') continue;

    $tasks[] = [
        'id' => $id,
        'name' => (string)($tool['name'] ?? $id),
        'description' => (string)($tool['description'] ?? ''),
        'tags' => is_array($tool['tags'] ?? null) ? array_values($tool['tags']) : [],
        'platforms' => is_array($tool['platforms'] ?? null) ? array_values($tool['platforms']) : [],
    ];
}

usort($tasks, function(array $a, array $b): int {
    return strcmp($a['id'], $b['id']);
});

echo json_encode(['ok' => true, 'tasks' => $tasks]);
