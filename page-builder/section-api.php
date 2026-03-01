<?php
/**
 * page-builder/section-api.php
 *
 * Manages the page manifest (pages/{name}/page.json) and individual section
 * JSON files. All write actions go through this file; the file system is the
 * source of truth.
 *
 * GET actions:
 *   ?action=get_page&page=<name>            - returns page.json
 *
 * POST actions (Content-Type: application/json body):
 *   ?action=reorder&page=<name>             - body: { sections: [{id,file,type,label},...] }
 *   ?action=add_section&page=<name>         - body: { template: "sections/hero.json", label: "Hero" }
 *   ?action=remove_section&page=<name>      - body: { id: "pb-sec-abc123" }
 *   ?action=update_section_json&page=<name> - body: { id: "pb-sec-abc123", content: {...} }
 *   ?action=rename_section&page=<name>      - body: { id: "pb-sec-abc123", label: "New Name" }
 */

declare(strict_types=1);

header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

function apiErr(string $msg, int $code = 400): void {
    error_log('[section-api] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

function apiOk(array $extra = []): void {
    echo json_encode(array_merge(['ok' => true], $extra));
    exit;
}

// ---- Helpers -------------------------------------------------------------- //

function validatePageName(string $name): bool {
    return (bool) preg_match('/^[a-z0-9_-]+$/i', $name);
}

function loadPageManifest(string $pageDir): array {
    $path = $pageDir . '/page.json';
    if (!file_exists($path)) {
        // Migrate: build manifest from existing files
        return buildMigratedManifest($pageDir);
    }
    $raw = file_get_contents($path);
    if ($raw === false) {
        error_log('[section-api] Cannot read page.json in: ' . $pageDir);
        return ['title' => basename($pageDir), 'sections' => []];
    }
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[section-api] Invalid JSON in page.json: ' . json_last_error_msg());
        return ['title' => basename($pageDir), 'sections' => []];
    }
    return $data;
}

function buildMigratedManifest(string $pageDir): array {
    $sections = [];
    $pageName = basename($pageDir);

    if (file_exists($pageDir . '/header.json')) {
        $sections[] = ['id' => 'pb-header', 'file' => 'header.json', 'type' => 'header', 'label' => 'Header'];
    }

    $sectionFiles = glob($pageDir . '/section-*.json');
    if ($sectionFiles) {
        natsort($sectionFiles);
        foreach ($sectionFiles as $sf) {
            $base     = basename($sf);
            // Strip extension, then strip leading "section-N" prefix for a readable label
            $stem     = pathinfo($base, PATHINFO_FILENAME);
            $stripped = preg_replace('/^section-\d+-?/', '', $stem);
            $label    = trim(ucwords(str_replace(['-', '_'], ' ', $stripped)));
            if ($label === '') $label = 'Section';
            $sections[] = [
                'id'    => 'pb-' . $stem,
                'file'  => $base,
                'type'  => 'section',
                'label' => $label,
            ];
        }
    }

    if (file_exists($pageDir . '/footer.json')) {
        $sections[] = ['id' => 'pb-footer', 'file' => 'footer.json', 'type' => 'footer', 'label' => 'Footer'];
    }

    return ['title' => ucfirst($pageName), 'sections' => $sections];
}

function savePageManifest(string $pageDir, array $manifest): bool {
    $path = $pageDir . '/page.json';
    $written = file_put_contents($path, json_encode($manifest, JSON_PRETTY_PRINT));
    if ($written === false) {
        error_log('[section-api] Failed to write page.json to: ' . $pageDir);
        return false;
    }
    return true;
}

function generateSectionId(): string {
    return 'pb-sec-' . substr(bin2hex(random_bytes(4)), 0, 8);
}

// ---- Routing -------------------------------------------------------------- //

$action = $_GET['action'] ?? '';
$page   = $_GET['page']   ?? '';

if (!$page || !validatePageName($page)) {
    apiErr('Missing or invalid page name');
}

$pagesDir = __DIR__ . '/pages';
$pageDir  = $pagesDir . '/' . $page;

if (!is_dir($pageDir)) {
    apiErr('Page not found: ' . $page, 404);
}

// ---- GET: get_section_file ------------------------------------------------ //
// Returns the raw JSON content of a single section file within a page.
// ?action=get_section_file&page=<name>&id=<sectionId>
if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'get_section_file') {
    $id = $_GET['id'] ?? '';
    if (!$id) {
        apiErr('Missing "id" parameter');
    }

    $manifest = loadPageManifest($pageDir);
    $entry    = null;
    foreach ($manifest['sections'] as $s) {
        if ($s['id'] === $id) { $entry = $s; break; }
    }
    if (!$entry) {
        apiErr('Section not found: ' . $id, 404);
    }

    // Guard against path traversal in recorded file name
    $file = $entry['file'];
    if (strpos($file, '/') !== false || strpos($file, '..') !== false) {
        apiErr('Invalid section file path', 400);
    }

    $filePath = $pageDir . '/' . $file;
    if (!file_exists($filePath)) {
        apiErr('Section file not found on disk: ' . $file, 404);
    }

    $raw = file_get_contents($filePath);
    if ($raw === false) {
        apiErr('Cannot read section file: ' . $file, 500);
    }

    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        apiErr('Section file contains invalid JSON: ' . json_last_error_msg(), 500);
    }

    error_log('[section-api] get_section_file id=' . $id . ' file=' . $file . ' page=' . $page);
    apiOk(['data' => $data]);
}

// ---- GET: get_page -------------------------------------------------------- //
if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    if ($action !== 'get_page') {
        apiErr('Unknown GET action: ' . $action);
    }

    $manifest = loadPageManifest($pageDir);

    // Auto-save migrated manifest so it persists
    if (!file_exists($pageDir . '/page.json')) {
        savePageManifest($pageDir, $manifest);
        error_log('[section-api] Migrated manifest written for page: ' . $page);
    }

    apiOk(['data' => $manifest]);
}

// ---- POST actions --------------------------------------------------------- //
if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    apiErr('Method not allowed', 405);
}

$raw = file_get_contents('php://input');
$body = $raw ? json_decode($raw, true) : [];
if (json_last_error() !== JSON_ERROR_NONE) {
    apiErr('Invalid request JSON: ' . json_last_error_msg());
}
$body = $body ?? [];

// ---- POST: reorder -------------------------------------------------------- //
if ($action === 'reorder') {
    $newOrder = $body['sections'] ?? null;
    if (!is_array($newOrder)) {
        apiErr('"sections" array is required');
    }

    // Validate each entry
    foreach ($newOrder as $i => $s) {
        if (empty($s['id']) || empty($s['file']) || empty($s['type'])) {
            apiErr('sections[' . $i . '] missing required id/file/type fields');
        }
        // Prevent path traversal in file name
        if (strpos($s['file'], '/') !== false || strpos($s['file'], '..') !== false) {
            apiErr('Invalid file path in section: ' . $s['file']);
        }
    }

    $manifest = loadPageManifest($pageDir);
    $manifest['sections'] = array_values($newOrder);
    if (!savePageManifest($pageDir, $manifest)) {
        apiErr('Failed to save page manifest', 500);
    }

    error_log('[section-api] Reordered ' . count($newOrder) . ' sections for page: ' . $page);
    apiOk(['sections' => $manifest['sections']]);
}

// ---- POST: add_section ---------------------------------------------------- //
if ($action === 'add_section') {
    $templatePath = $body['template'] ?? '';
    $label        = trim($body['label'] ?? '');

    if (!$templatePath) {
        apiErr('"template" path is required');
    }

    // Security: no path traversal
    $templatePath = ltrim(str_replace('..', '', $templatePath), '/\\');
    $fullTplPath  = __DIR__ . '/sections/' . $templatePath;

    if (!file_exists($fullTplPath)) {
        apiErr('Template not found: ' . $templatePath, 404);
    }

    $tplRaw = file_get_contents($fullTplPath);
    if ($tplRaw === false) {
        apiErr('Cannot read template file', 500);
    }

    $tplData = json_decode($tplRaw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        apiErr('Template JSON is invalid: ' . json_last_error_msg(), 500);
    }

    // Determine type from template
    $meta = $tplData['_meta'] ?? [];
    $type = $meta['type'] ?? $tplData['type'] ?? 'section';

    // If type is header or footer, use the reserved file names
    if ($type === 'header') {
        $fileName = 'header.json';
        $secId    = 'pb-header';
    } elseif ($type === 'footer') {
        $fileName = 'footer.json';
        $secId    = 'pb-footer';
    } else {
        $secId    = generateSectionId();
        $fileName = $secId . '.json';
    }

    // Strip _meta from saved file
    unset($tplData['_meta']);

    // Stamp a unique section id into id field if it is a section type
    if ($type === 'section' && !empty($tplData['id'])) {
        // keep original id so blocks reference it correctly, but prefix to avoid collision
        $tplData['id'] = $secId . '-' . $tplData['id'];
    }

    // Save JSON to page directory
    $destPath = $pageDir . '/' . $fileName;
    if (file_put_contents($destPath, json_encode($tplData, JSON_PRETTY_PRINT)) === false) {
        apiErr('Cannot write section file: ' . $fileName, 500);
    }

    if (!$label) {
        $label = $meta['name'] ?? ucwords(str_replace(['-','_'], ' ', $type));
    }

    // Add to manifest
    $manifest = loadPageManifest($pageDir);

    // For header add at top, footer at bottom; sections in the middle
    $entry = ['id' => $secId, 'file' => $fileName, 'type' => $type, 'label' => $label];

    if ($type === 'header') {
        // Replace existing header entry or prepend
        $existing = array_filter($manifest['sections'], fn($s) => $s['type'] === 'header');
        if ($existing) {
            foreach ($manifest['sections'] as &$s) {
                if ($s['type'] === 'header') { $s = $entry; break; }
            }
            unset($s);
        } else {
            array_unshift($manifest['sections'], $entry);
        }
    } elseif ($type === 'footer') {
        // Replace existing footer entry or append
        $existing = array_filter($manifest['sections'], fn($s) => $s['type'] === 'footer');
        if ($existing) {
            foreach ($manifest['sections'] as &$s) {
                if ($s['type'] === 'footer') { $s = $entry; break; }
            }
            unset($s);
        } else {
            $manifest['sections'][] = $entry;
        }
    } else {
        // Insert before footer (if any), else append
        $footerIdx = null;
        foreach ($manifest['sections'] as $i => $s) {
            if ($s['type'] === 'footer') { $footerIdx = $i; break; }
        }
        if ($footerIdx !== null) {
            array_splice($manifest['sections'], $footerIdx, 0, [$entry]);
        } else {
            $manifest['sections'][] = $entry;
        }
    }

    if (!savePageManifest($pageDir, $manifest)) {
        apiErr('Failed to update page manifest', 500);
    }

    error_log('[section-api] Added section "' . $fileName . '" to page: ' . $page);
    apiOk(['section' => $entry, 'sections' => $manifest['sections']]);
}

// ---- POST: remove_section ------------------------------------------------- //
if ($action === 'remove_section') {
    $id = $body['id'] ?? '';
    if (!$id) {
        apiErr('"id" is required');
    }

    $manifest = loadPageManifest($pageDir);
    $removed  = null;
    $newSections = [];

    foreach ($manifest['sections'] as $s) {
        if ($s['id'] === $id) {
            $removed = $s;
        } else {
            $newSections[] = $s;
        }
    }

    if (!$removed) {
        apiErr('Section not found: ' . $id, 404);
    }

    $manifest['sections'] = array_values($newSections);
    if (!savePageManifest($pageDir, $manifest)) {
        apiErr('Failed to update page manifest', 500);
    }

    error_log('[section-api] Removed section id=' . $id . ' from page: ' . $page);
    apiOk(['removed' => $removed, 'sections' => $manifest['sections']]);
}

// ---- POST: update_section_json ------------------------------------------- //
if ($action === 'update_section_json') {
    $id      = $body['id']      ?? '';
    $content = $body['content'] ?? null;

    if (!$id || $content === null) {
        apiErr('"id" and "content" are required');
    }

    $manifest = loadPageManifest($pageDir);

    $entry = null;
    foreach ($manifest['sections'] as $s) {
        if ($s['id'] === $id) { $entry = $s; break; }
    }
    if (!$entry) {
        apiErr('Section not found: ' . $id, 404);
    }

    // Validate content is an array (decoded JSON object)
    if (!is_array($content)) {
        apiErr('"content" must be a JSON object');
    }

    $filePath = $pageDir . '/' . $entry['file'];
    if (file_put_contents($filePath, json_encode($content, JSON_PRETTY_PRINT)) === false) {
        apiErr('Cannot write section file: ' . $entry['file'], 500);
    }

    error_log('[section-api] Updated JSON for section id=' . $id . ' file=' . $entry['file'] . ' page=' . $page);
    apiOk(['id' => $id, 'file' => $entry['file']]);
}

// ---- POST: rename_section ------------------------------------------------- //
if ($action === 'rename_section') {
    $id    = $body['id']    ?? '';
    $label = trim($body['label'] ?? '');

    if (!$id || !$label) {
        apiErr('"id" and "label" are required');
    }

    $manifest = loadPageManifest($pageDir);
    $found    = false;

    foreach ($manifest['sections'] as &$s) {
        if ($s['id'] === $id) {
            $s['label'] = $label;
            $found = true;
            break;
        }
    }
    unset($s);

    if (!$found) {
        apiErr('Section not found: ' . $id, 404);
    }

    if (!savePageManifest($pageDir, $manifest)) {
        apiErr('Failed to update page manifest', 500);
    }

    error_log('[section-api] Renamed section id=' . $id . ' to "' . $label . '" page=' . $page);
    apiOk(['sections' => $manifest['sections']]);
}

apiErr('Unknown action: ' . $action);
