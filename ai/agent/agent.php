<?php
/**
 * Agent Orchestrator
 * Single entry point for all agent actions.
 * Routes to the correct sub-module based on the "action" field.
 *
 * POST body (JSON):
 *   action      string  Required. One of:
 *                         save_version | get_versions | navigate | diff
 *                         | outline | context_get | context_learn
 *                         | providers | stream_check
 *   ...action-specific fields...
 */

require_once __DIR__ . '/../config.php';
require_once __DIR__ . '/db.php';
require_once __DIR__ . '/diff.php';
require_once __DIR__ . '/outline.php';
require_once __DIR__ . '/context/context.php';
require_once __DIR__ . '/../../style-sheets/css-outline.php';
require_once __DIR__ . '/../../style-sheets/fuzzy-search.php';

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] !== 'POST') {
    abort('Only POST is accepted.', 405);
}

$body = json_decode(file_get_contents('php://input'), true);
if (json_last_error() !== JSON_ERROR_NONE) {
    abort('Malformed JSON body.');
}

$action = $body['action'] ?? '';

switch ($action) {

    // -------------------------------------------------------------------------
    // Version management
    // -------------------------------------------------------------------------

    case 'save_version': {
        $path    = required($body, 'file_path');
        $content = required($body, 'content');
        $label   = $body['label'] ?? '';
        $id      = AgentDB::saveVersion($path, $content, $label);
        $versions = AgentDB::getAllVersions($path);
        $history  = AgentDB::getHistory($path);
        respond([
            'version_id' => $id,
            'versions'   => $versions,
            'history'    => $history,
        ]);
    }

    case 'get_versions': {
        $path     = required($body, 'file_path');
        $versions = AgentDB::getAllVersions($path);
        $history  = AgentDB::getHistory($path);
        respond(['versions' => $versions, 'history' => $history]);
    }

    case 'navigate': {
        $path      = required($body, 'file_path');
        $direction = $body['direction'] ?? 'back';
        if (!in_array($direction, ['back', 'forward'])) {
            abort('direction must be "back" or "forward".');
        }
        $version = AgentDB::navigate($path, $direction);
        if ($version === null) {
            respond(['blocked' => true, 'reason' => 'Cannot navigate ' . $direction . ' from here.']);
        }
        $history = AgentDB::getHistory($path);
        respond(['version' => $version, 'history' => $history]);
    }

    case 'current_version': {
        $path    = required($body, 'file_path');
        $version = AgentDB::currentVersion($path);
        respond(['version' => $version]);
    }

    // -------------------------------------------------------------------------
    // Diff
    // -------------------------------------------------------------------------

    case 'diff': {
        $path = required($body, 'file_path');

        if (isset($body['old_text'], $body['new_text'])) {
            $oldText = $body['old_text'];
            $newText = $body['new_text'];
        } else {
            // Default: diff last two versions
            $versions = AgentDB::getAllVersions($path);
            if (count($versions) < 2) {
                respond(['hunks' => [], 'summary' => ['added' => 0, 'removed' => 0], 'message' => 'Only one version available.']);
            }
            $newText = $versions[0]['content'];
            $oldText = $versions[1]['content'];
        }

        $ctx    = (int) ($body['context'] ?? 3);
        $hunks  = AgentDiff::diff($oldText, $newText, $ctx);
        $sum    = AgentDiff::summary($hunks);
        $html   = AgentDiff::toHtmlRows($hunks);
        respond(['hunks' => $hunks, 'summary' => $sum, 'html' => $html]);
    }

    // -------------------------------------------------------------------------
    // Outline
    // -------------------------------------------------------------------------

    case 'outline': {
        $path    = required($body, 'file_path');
        $content = required($body, 'content');
        $nodes   = AgentOutline::extract($content, $path);
        $html    = AgentOutline::toHtml($nodes);
        respond(['nodes' => $nodes, 'html' => $html]);
    }

    // -------------------------------------------------------------------------
    // Context
    // -------------------------------------------------------------------------

    case 'context_get': {
        $path = required($body, 'file_path');
        respond(AgentContext::load($path));
    }

    case 'context_learn': {
        $path     = required($body, 'file_path');
        $pattern  = required($body, 'pattern');
        $category = $body['category'] ?? 'style';
        AgentContext::learnPattern($path, $pattern, $category);
        respond(['ok' => true]);
    }

    case 'context_avoid': {
        $path = required($body, 'file_path');
        $item = required($body, 'item');
        AgentContext::avoid($path, $item);
        respond(['ok' => true]);
    }

    case 'context_all': {
        respond(['contexts' => AgentContext::allContexts()]);
    }

    // -------------------------------------------------------------------------
    // Provider / model info
    // -------------------------------------------------------------------------

    case 'providers': {
        $config = json_decode(file_get_contents(__DIR__ . '/../config.json'), true);
        $out    = [];
        foreach ($config['providers'] as $slug => $cfg) {
            $out[$slug] = [
                'name'              => $cfg['name'],
                'default_model'     => $cfg['default_model'],
                'models'            => $cfg['models'] ?? [],
                'supports_streaming'=> $cfg['supports_streaming'] ?? false,
            ];
        }
        respond(['providers' => $out]);
    }

    case 'stream_check': {
        $config = json_decode(file_get_contents(__DIR__ . '/../config.json'), true);
        $slug   = $body['provider'] ?? '';
        $out    = $config['providers'][$slug]['supports_streaming'] ?? false;
        respond(['provider' => $slug, 'streaming' => $out]);
    }

    // -------------------------------------------------------------------------
    // File list (all tracked files)
    // -------------------------------------------------------------------------

    case 'files': {
        respond(['files' => AgentDB::allFiles()]);
    }

    // -------------------------------------------------------------------------
    // Apply agent result (save after AI edits)
    // -------------------------------------------------------------------------

    case 'apply': {
        $path    = required($body, 'file_path');
        $content = required($body, 'content');
        $summary = $body['summary']   ?? 'AI edit';
        $model   = $body['model']     ?? '';

        $id  = AgentDB::saveVersion($path, $content, $summary);
        AgentContext::addChange($path, $summary, $id, $model);

        $all     = AgentDB::getAllVersions($path);
        $history = AgentDB::getHistory($path);
        respond(['version_id' => $id, 'versions' => $all, 'history' => $history]);
    }

    // -------------------------------------------------------------------------
    // Run command lint/check
    // -------------------------------------------------------------------------

    case 'run_command': {
        $path    = required($body, 'file_path');
        $ext     = strtolower(pathinfo($path, PATHINFO_EXTENSION));
        $prompts = json_decode(file_get_contents(__DIR__ . '/prompts.json'), true);
        $tpl     = $prompts['run_commands'][$ext] ?? null;

        if (!$tpl) {
            respond(['output' => 'No run command configured for .' . $ext . ' files.', 'exit_code' => -1]);
        }

        // Only allow the command to run against the DB version (not arbitrary paths)
        $version = AgentDB::currentVersion($path);
        if (!$version) {
            respond(['output' => 'No stored version found.', 'exit_code' => -1]);
        }

        $tmpFile = sys_get_temp_dir() . '/agent_run_' . uniqid() . '.' . $ext;
        file_put_contents($tmpFile, $version['content']);

        $cmd    = str_replace('{file}', escapeshellarg($tmpFile), $tpl);
        $output = shell_exec($cmd . ' 2>&1');
        $code   = 0;
        unlink($tmpFile);

        respond(['output' => $output ?? '', 'exit_code' => $code, 'command' => $cmd]);
    }

    // -------------------------------------------------------------------------
    // CSS Outline -- structured extraction for model consumption
    // -------------------------------------------------------------------------

    case 'css_outline': {
        $path   = required($body, 'file_path');
        $format = $body['format'] ?? 'json'; // json or text

        $styleSheetsDir = realpath(__DIR__ . '/../../style-sheets');
        $cssFile = $styleSheetsDir . '/' . basename($path);

        if (!file_exists($cssFile)) {
            abort('CSS file not found: ' . basename($path));
        }

        $css     = file_get_contents($cssFile);
        $outline = CSSOutline::extract($css, basename($path));

        if ($format === 'text') {
            respond(['outline' => CSSOutline::toText($outline)]);
        }
        respond(['outline' => $outline]);
    }

    // -------------------------------------------------------------------------
    // Fuzzy Search -- find related info across themes, rules, learn
    // -------------------------------------------------------------------------

    case 'fuzzy_search': {
        $query  = required($body, 'query');
        $format = $body['format'] ?? 'json'; // json or text

        $styleSheetsDir = realpath(__DIR__ . '/../../style-sheets');
        $results = FuzzySearch::search($query, $styleSheetsDir);

        if ($format === 'text') {
            respond(['results' => FuzzySearch::toText($results)]);
        }
        respond(['results' => $results]);
    }

    // -------------------------------------------------------------------------
    // Backup -- create timestamped backups of learn.json and rules.json
    // -------------------------------------------------------------------------

    case 'backup': {
        $target = $body['target'] ?? 'all'; // all, learn, rules
        $styleSheetsDir = realpath(__DIR__ . '/../../style-sheets');
        $backupDir = $styleSheetsDir . '/backups';
        if (!is_dir($backupDir)) {
            mkdir($backupDir, 0755, true);
        }

        $ts      = date('Y-m-d_His');
        $backed  = [];

        $targets = [];
        if ($target === 'all' || $target === 'learn') {
            $targets[] = 'learn.json';
        }
        if ($target === 'all' || $target === 'rules') {
            $targets[] = 'rules.json';
        }

        foreach ($targets as $file) {
            $src = $styleSheetsDir . '/' . $file;
            if (file_exists($src)) {
                $dst = $backupDir . '/' . pathinfo($file, PATHINFO_FILENAME) . '_' . $ts . '.json';
                copy($src, $dst);
                $backed[] = ['file' => $file, 'backup' => basename($dst)];
            }
        }

        respond(['ok' => true, 'backups' => $backed, 'timestamp' => $ts]);
    }

    // -------------------------------------------------------------------------
    // Agent modes info
    // -------------------------------------------------------------------------

    case 'agent_modes': {
        $prompts = json_decode(file_get_contents(__DIR__ . '/prompts.json'), true);
        $modes   = $prompts['agent_modes'] ?? [];
        respond(['modes' => $modes]);
    }

    // -------------------------------------------------------------------------
    // List available CSS themes
    // -------------------------------------------------------------------------

    case 'list_themes': {
        $styleSheetsDir = realpath(__DIR__ . '/../../style-sheets');
        $cssFiles = glob($styleSheetsDir . '/*.css');
        $themes = [];
        foreach ($cssFiles as $f) {
            $name = basename($f, '.css');
            $css  = file_get_contents($f);
            $outline = CSSOutline::extract($css, basename($f));
            $themes[] = [
                'name'       => $name,
                'file'       => basename($f),
                'prefix'     => $outline['prefix'],
                'body_class' => $outline['body_class'],
                'stats'      => $outline['stats'],
            ];
        }
        respond(['themes' => $themes]);
    }

    default:
        abort('Unknown action: ' . htmlspecialchars($action, ENT_QUOTES, 'UTF-8'));
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

function respond(array $data, int $code = 200): never
{
    http_response_code($code);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function abort(string $msg, int $code = 400): never
{
    respond(['error' => $msg], $code);
}

function required(array $body, string $key): string
{
    if (!isset($body[$key]) || $body[$key] === '') {
        abort("Missing required field: $key");
    }
    return $body[$key];
}
