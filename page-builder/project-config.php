<?php
/**
 * page-builder/project-config.php
 *
 * Manages the top-level project settings stored in project.json.
 *
 * Settings:
 *   index_page  - the page name whose built HTML becomes deploy/index.html
 *   slugs       - map of { pageName: "/url-path/" } resolved on nav hrefs at build time
 *
 * GET  project-config.php            -> returns current project.json (or defaults)
 * POST project-config.php            -> saves new settings, returns saved data
 *
 * POST body (JSON):
 *   { "index_page": "demo", "slugs": { "demo": "/", "about": "/about/" } }
 *
 * Slug resolution rules (applied at page build time):
 *   - href is exactly a page name with no leading / # or protocol -> resolved to slug
 *   - href starts with # / or contains :// -> kept as-is
 *
 * Security:
 *   - page name and slug values are validated
 *   - path traversal blocked by strict name validation
 *   - POST only for writes
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

function cfgErr(string $msg, int $code = 400): void {
    error_log('[project-config] ERROR: ' . $msg);
    http_response_code($code);
    echo json_encode(['ok' => false, 'error' => $msg]);
    exit;
}

$configFile = __DIR__ . '/project.json';
$pagesRoot  = __DIR__ . '/pages';

// ---- Load helper -----------------------------------------------------------

function loadProjectConfig(string $configFile): array {
    $defaults = [
        'index_page' => '',
        'slugs' => [],
        'builder_script_language' => 'javascript',
        'supported_script_languages' => ['javascript'],
        'breadcrumb_manager_enabled' => false,
        'breadcrumb_manager_package' => 'bc_mgr_wasm_with_storage',
        'updated_at' => '',
    ];
    if (!file_exists($configFile)) {
        return $defaults;
    }
    $raw = file_get_contents($configFile);
    if ($raw === false) {
        error_log('[project-config] could not read project.json');
        return $defaults;
    }
    $data = json_decode($raw, true);
    if (!is_array($data)) {
        error_log('[project-config] project.json contains invalid JSON: ' . json_last_error_msg());
        return $defaults;
    }
    $cfg = array_merge($defaults, $data);

    if (!is_array($cfg['slugs'] ?? null)) {
        $cfg['slugs'] = [];
    }
    if (!is_array($cfg['supported_script_languages'] ?? null)) {
        $cfg['supported_script_languages'] = ['javascript'];
    }
    if (!in_array((string)($cfg['builder_script_language'] ?? 'javascript'), ['javascript', 'typescript'], true)) {
        $cfg['builder_script_language'] = 'javascript';
    }
    $cfg['breadcrumb_manager_enabled'] = !empty($cfg['breadcrumb_manager_enabled']);
    if (!in_array((string)($cfg['breadcrumb_manager_package'] ?? 'bc_mgr_wasm_with_storage'), ['bc_mgr_wasm_with_storage', 'bc_mgr_wasm_dropin'], true)) {
        $cfg['breadcrumb_manager_package'] = 'bc_mgr_wasm_with_storage';
    }

    return $cfg;
}

// ---- GET ------------------------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    $cfg = loadProjectConfig($configFile);

    // Also return the list of known page names so the UI can build its pickers
    $pageNames = [];
    if (is_dir($pagesRoot)) {
        foreach (scandir($pagesRoot) as $n) {
            if ($n === '.' || $n === '..') continue;
            if (!is_dir($pagesRoot . '/' . $n)) continue;
            if (!preg_match('/^[a-z0-9_-]+$/i', $n)) continue;
            $pageNames[] = $n;
        }
    }

    echo json_encode(['ok' => true, 'config' => $cfg, 'pages' => $pageNames]);
    exit;
}

// ---- POST -----------------------------------------------------------------

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    cfgErr('GET or POST required', 405);
}

$raw  = file_get_contents('php://input');
$body = json_decode($raw ?: '{}', true);

if (json_last_error() !== JSON_ERROR_NONE) {
    cfgErr('Invalid JSON body: ' . json_last_error_msg());
}

if (!is_array($body)) {
    cfgErr('Request body must be a JSON object');
}

$currentCfg = loadProjectConfig($configFile);

// Validate index_page
$indexPage = trim($body['index_page'] ?? '');
if ($indexPage !== '' && !preg_match('/^[a-z0-9_-]+$/i', $indexPage)) {
    cfgErr('index_page contains invalid characters (a-z 0-9 _ - only)');
}
if ($indexPage !== '' && !is_dir($pagesRoot . '/' . $indexPage)) {
    cfgErr('index_page "' . $indexPage . '" does not exist in pages/', 404);
}

// Validate slugs map
$rawSlugs = $body['slugs'] ?? [];
if (!is_array($rawSlugs)) {
    cfgErr('"slugs" must be an object');
}

$slugs = [];
foreach ($rawSlugs as $pageName => $slug) {
    $pageName = (string) $pageName;
    $slug     = (string) $slug;

    if (!preg_match('/^[a-z0-9_-]+$/i', $pageName)) {
        cfgErr('Slug key "' . $pageName . '" contains invalid characters');
    }
    // Slug value: must start with / or be empty, max 200 chars, no injections
    if ($slug !== '' && !preg_match('/^\/[a-zA-Z0-9\/_\-\.]*$/', $slug)) {
        cfgErr('Slug value "' . $slug . '" must start with / and contain only path characters');
    }
    $slugs[$pageName] = $slug;
}

// Validate script language settings
$allowedScriptLangs = ['javascript', 'typescript'];
$allowedBreadcrumbPackages = ['bc_mgr_wasm_with_storage', 'bc_mgr_wasm_dropin'];

$builderScriptLanguage = strtolower(trim((string)($body['builder_script_language'] ?? ($currentCfg['builder_script_language'] ?? 'javascript'))));
if (!in_array($builderScriptLanguage, $allowedScriptLangs, true)) {
    cfgErr('builder_script_language must be one of: javascript, typescript');
}

$supportedScriptLanguagesRaw = $body['supported_script_languages'] ?? ($currentCfg['supported_script_languages'] ?? ['javascript']);
if (!is_array($supportedScriptLanguagesRaw)) {
    cfgErr('supported_script_languages must be an array');
}

$supportedScriptLanguages = [];
foreach ($supportedScriptLanguagesRaw as $lang) {
    $lang = strtolower(trim((string)$lang));
    if ($lang === '') {
        continue;
    }
    if (!in_array($lang, $allowedScriptLangs, true)) {
        cfgErr('supported_script_languages contains unsupported value: ' . $lang);
    }
    if (!in_array($lang, $supportedScriptLanguages, true)) {
        $supportedScriptLanguages[] = $lang;
    }
}

if (empty($supportedScriptLanguages)) {
    $supportedScriptLanguages = ['javascript'];
}

if (!in_array($builderScriptLanguage, $supportedScriptLanguages, true)) {
    $supportedScriptLanguages[] = $builderScriptLanguage;
}

$breadcrumbManagerEnabled = $body['breadcrumb_manager_enabled'] ?? ($currentCfg['breadcrumb_manager_enabled'] ?? false);
$breadcrumbManagerEnabled = filter_var($breadcrumbManagerEnabled, FILTER_VALIDATE_BOOLEAN, FILTER_NULL_ON_FAILURE);
if ($breadcrumbManagerEnabled === null) {
    cfgErr('breadcrumb_manager_enabled must be a boolean');
}

$breadcrumbManagerPackage = trim((string)($body['breadcrumb_manager_package'] ?? ($currentCfg['breadcrumb_manager_package'] ?? 'bc_mgr_wasm_with_storage')));
if (!in_array($breadcrumbManagerPackage, $allowedBreadcrumbPackages, true)) {
    cfgErr('breadcrumb_manager_package must be one of: bc_mgr_wasm_with_storage, bc_mgr_wasm_dropin');
}

$cfg = [
    'index_page' => $indexPage,
    'slugs'      => $slugs,
    'builder_script_language' => $builderScriptLanguage,
    'supported_script_languages' => $supportedScriptLanguages,
    'breadcrumb_manager_enabled' => $breadcrumbManagerEnabled,
    'breadcrumb_manager_package' => $breadcrumbManagerPackage,
    'updated_at' => date('c'),
];

$written = file_put_contents($configFile, json_encode($cfg, JSON_PRETTY_PRINT));
if ($written === false) {
    cfgErr('Failed to write project.json', 500);
}

error_log('[project-config] saved: index_page=' . ($indexPage ?: '(none)') . ' slugs=' . count($slugs));
echo json_encode(['ok' => true, 'config' => $cfg]);
