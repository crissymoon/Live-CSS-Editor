<?php
/**
 * page-builder/section-library.php
 *
 * API: returns the list of available section templates from page-builder/sections/.
 * Each template JSON may contain a "_meta" key with { name, type, description }.
 *
 * Actions (GET):
 *   ?action=list              - returns all templates grouped by type
 *   ?action=get&path=<relPath> - returns raw template JSON (path relative to sections/)
 *
 * CORS is open so the page-builder served by PHP's dev server can call it freely.
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

function libErr(string $msg, int $code = 400): void {
    error_log('[section-library] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

$action = $_GET['action'] ?? 'list';
$sectionsRoot = __DIR__ . '/sections';

if (!is_dir($sectionsRoot)) {
    libErr('sections/ directory not found', 500);
}

// ---- action: get ---------------------------------------------------------- //
if ($action === 'get') {
    $relPath = $_GET['path'] ?? '';
    if (!$relPath) {
        libErr('Missing path parameter');
    }

    // Security: no path traversal
    $relPath  = ltrim(str_replace('..', '', $relPath), '/\\');
    $fullPath = $sectionsRoot . '/' . $relPath;

    if (!file_exists($fullPath) || pathinfo($fullPath, PATHINFO_EXTENSION) !== 'json') {
        libErr('Template not found: ' . $relPath, 404);
    }

    $raw = file_get_contents($fullPath);
    if ($raw === false) {
        libErr('Cannot read template: ' . $relPath, 500);
    }

    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        libErr('Invalid JSON in template: ' . json_last_error_msg(), 500);
    }

    echo json_encode(['ok' => true, 'data' => $data]);
    exit;
}

// ---- action: list --------------------------------------------------------- //
if ($action !== 'list') {
    libErr('Unknown action: ' . $action);
}

$templates = [];

// Recursively scan sections/ subdirectories
$dir = new RecursiveDirectoryIterator($sectionsRoot, FilesystemIterator::SKIP_DOTS);
$it  = new RecursiveIteratorIterator($dir);

foreach ($it as $file) {
    if ($file->getExtension() !== 'json') continue;

    $fullPath = $file->getPathname();
    $relPath  = ltrim(str_replace($sectionsRoot, '', $fullPath), '/\\');

    $raw = file_get_contents($fullPath);
    if ($raw === false) {
        error_log('[section-library] Cannot read: ' . $fullPath);
        continue;
    }

    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[section-library] Invalid JSON in: ' . $fullPath . ' - ' . json_last_error_msg());
        continue;
    }

    // Extract metadata - prefer _meta block, fall back to top-level keys
    $meta = $data['_meta'] ?? [];
    $type = $meta['type'] ?? $data['type'] ?? 'section';
    $name = $meta['name'] ?? ucwords(str_replace(['-', '_', '.json'], [' ', ' ', ''], basename($fullPath)));
    $desc = $meta['description'] ?? '';

    $templates[] = [
        'path'        => str_replace('\\', '/', $relPath),
        'name'        => $name,
        'type'        => $type,
        'description' => $desc,
    ];
}

// Sort by type then name
usort($templates, function ($a, $b) {
    $typePriority = ['header' => 0, 'section' => 1, 'form' => 2, 'panel' => 3, 'footer' => 4];
    $ta = $typePriority[$a['type']] ?? 10;
    $tb = $typePriority[$b['type']] ?? 10;
    if ($ta !== $tb) return $ta - $tb;
    return strcasecmp($a['name'], $b['name']);
});

echo json_encode(['ok' => true, 'data' => $templates]);
