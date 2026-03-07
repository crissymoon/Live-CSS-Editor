<?php
/**
 * scripts/file-map.php
 *
 * File-structure validator and reporter for Crissy's Style Tool.
 * Reads style-tool.config.json and walks the project root to produce a
 * structured report of what is present, missing, or unexpected.
 *
 * Usage (CLI):
 *   php scripts/file-map.php [--root /path/to/project] [--mode dev|build] [--json]
 *
 * Usage (PHP include):
 *   require_once __DIR__ . '/scripts/file-map.php';
 *   $map    = new FileMap('/path/to/project');
 *   $report = $map->run();
 *   echo $map->renderText($report);
 *
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License
 */

class FileMap
{
    /** @var string absolute path to the project root */
    private string $root;

    /** @var array decoded style-tool.config.json, or [] if not present */
    private array $config = [];

    /** @var string active mode: "dev" or "build" */
    private string $mode = 'dev';

    // ------------------------------------------------------------------
    // Construction
    // ------------------------------------------------------------------

    public function __construct(string $root = '', string $mode = '')
    {
        $this->root = $root ?: $this->detectRoot();
        $this->loadConfig();
        $this->mode = $mode ?: ($this->config['mode'] ?? 'dev');
    }

    // ------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------

    /**
     * Run the full validation scan.
     * Returns a structured array with keys:
     *   root, mode, config_found, config_valid,
     *   entries  (array of entry results)
     *   summary  (counts of ok / missing / optional_missing / unexpected)
     */
    public function run(): array
    {
        $entries = [];

        $entries[] = $this->checkFile('index.php', 'required', 'entry-point');
        $entries[] = $this->checkFile('style.css',  'required', 'base-styles');

        $map = $this->config['map'] ?? [];

        // -- top-level files from config map ----------------------------
        foreach ($map['files'] ?? [] as $f) {
            $path   = $f['path']   ?? '';
            $status = $f['status'] ?? 'optional';
            $role   = $f['role']   ?? '';
            if (in_array($path, ['index.php', 'style.css'])) continue; // already added
            $entries[] = $this->checkFile($path, $status, $role, $f['note'] ?? '');
        }

        // -- directories ------------------------------------------------
        foreach ($map['dirs'] ?? [] as $dirName => $dirCfg) {
            $entries = array_merge($entries, $this->checkDir($dirName, $dirCfg));
        }

        // -- vscode-bridge specific checks ------------------------------
        $entries = array_merge($entries, $this->checkBridge());

        // -- summary ----------------------------------------------------
        $ok = $missing = $opt_missing = $warn = 0;
        foreach ($entries as $e) {
            switch ($e['result']) {
                case 'ok':              $ok++;          break;
                case 'missing':         $missing++;     break;
                case 'optional_missing':$opt_missing++; break;
                case 'warn':            $warn++;        break;
            }
        }

        return [
            'root'         => $this->root,
            'mode'         => $this->mode,
            'config_found' => !empty($this->config),
            'config_valid' => !empty($this->config['version']),
            'entries'      => $entries,
            'summary'      => [
                'ok'              => $ok,
                'missing'         => $missing,
                'optional_missing'=> $opt_missing,
                'warnings'        => $warn,
                'total'           => count($entries),
                'pass'            => $missing === 0,
            ],
        ];
    }

    /**
     * Render a plain-text report suitable for the terminal.
     */
    public function renderText(array $report): string
    {
        $lines = [];
        $lines[] = str_repeat('-', 64);
        $lines[] = 'Style Tool File Map  [mode: ' . $report['mode'] . ']';
        $lines[] = 'Root: ' . $report['root'];
        $lines[] = 'Config: ' . ($report['config_found'] ? 'found' : 'NOT FOUND -- run setup.php');
        $lines[] = str_repeat('-', 64);

        foreach ($report['entries'] as $e) {
            $icon = match ($e['result']) {
                'ok'               => '[+]',
                'missing'          => '[!]',
                'optional_missing' => '[-]',
                'warn'             => '[~]',
                default            => '[ ]',
            };
            $line = sprintf('%-5s %-45s %s', $icon, $e['path'], $e['role'] ?? '');
            if (!empty($e['note'])) {
                $line .= '  -- ' . $e['note'];
            }
            $lines[] = $line;
        }

        $s = $report['summary'];
        $lines[] = str_repeat('-', 64);
        $lines[] = sprintf(
            'OK: %d  Missing: %d  Optional missing: %d  Warnings: %d',
            $s['ok'], $s['missing'], $s['optional_missing'], $s['warnings']
        );
        $lines[] = $s['pass'] ? 'PASS -- all required files present.' : 'FAIL -- required files missing.';

        return implode("\n", $lines) . "\n";
    }

    /**
     * Render an HTML report suitable for embedding in setup.php.
     */
    public function renderHtml(array $report): string
    {
        $s = $report['summary'];
        $passClass = $s['pass'] ? 'fm-pass' : 'fm-fail';
        $html = '<div class="file-map">';
        $html .= '<div class="fm-root">Root: <code>' . htmlspecialchars($report['root']) . '</code></div>';
        $html .= '<div class="fm-mode">Mode: <code>' . htmlspecialchars($report['mode']) . '</code></div>';

        if (!$report['config_found']) {
            $html .= '<div class="fm-warn fm-row">style-tool.config.json not found -- run setup to create it.</div>';
        }

        $html .= '<table class="fm-table">';
        $html .= '<tr><th>Status</th><th>Path</th><th>Role</th><th>Note</th></tr>';

        foreach ($report['entries'] as $e) {
            $cls = match ($e['result']) {
                'ok'               => 'fm-ok',
                'missing'          => 'fm-missing',
                'optional_missing' => 'fm-optional',
                'warn'             => 'fm-warn',
                default            => '',
            };
            $icon = match ($e['result']) {
                'ok'               => '&#10003;',
                'missing'          => '&#10007;',
                'optional_missing' => '&#8212;',
                'warn'             => '&#9651;',
                default            => '?',
            };
            $html .= sprintf(
                '<tr class="%s"><td>%s</td><td><code>%s</code></td><td>%s</td><td>%s</td></tr>',
                $cls,
                $icon,
                htmlspecialchars($e['path']),
                htmlspecialchars($e['role'] ?? ''),
                htmlspecialchars($e['note'] ?? '')
            );
        }

        $html .= '</table>';
        $html .= sprintf(
            '<div class="fm-summary %s">OK: %d &nbsp; Missing: %d &nbsp; Optional missing: %d &nbsp; Warnings: %d &nbsp;&mdash;&nbsp; %s</div>',
            $passClass,
            $s['ok'], $s['missing'], $s['optional_missing'], $s['warnings'],
            $s['pass'] ? 'All required files present.' : 'Required files missing.'
        );
        $html .= '</div>';
        return $html;
    }

    // ------------------------------------------------------------------
    // Config helpers
    // ------------------------------------------------------------------

    public function getConfig(): array  { return $this->config; }
    public function getMode(): string   { return $this->mode; }
    public function getRoot(): string   { return $this->root; }

    public function setMode(string $mode): void
    {
        $this->mode = $mode;
    }

    /**
     * Merge an array of overrides into the in-memory config and persist to disk.
     */
    public function saveConfig(array $overrides): bool
    {
        $this->config = array_replace_recursive($this->config, $overrides);
        $path = $this->root . '/style-tool.config.json';
        $json = json_encode($this->config, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
        return (bool) file_put_contents($path, $json . "\n");
    }

    /**
     * Return the mode-specific config block (modes.dev or modes.build).
     */
    public function modeConfig(): array
    {
        return $this->config['modes'][$this->mode] ?? [];
    }

    /**
     * Return the value of a feature flag.
     */
    public function featureEnabled(string $flag): bool
    {
        return (bool) ($this->config['feature_flags'][$flag] ?? true);
    }

    // ------------------------------------------------------------------
    // Private helpers
    // ------------------------------------------------------------------

    private function detectRoot(): string
    {
        // Walk up from this file until we find style-tool.config.json or index.php
        $dir = dirname(__DIR__);
        for ($i = 0; $i < 5; $i++) {
            if (file_exists($dir . '/style-tool.config.json') ||
                file_exists($dir . '/index.php')) {
                return realpath($dir);
            }
            $parent = dirname($dir);
            if ($parent === $dir) break;
            $dir = $parent;
        }
        return dirname(__DIR__);
    }

    private function loadConfig(): void
    {
        $path = $this->root . '/style-tool.config.json';
        if (!file_exists($path)) {
            $this->config = [];
            return;
        }
        $raw = file_get_contents($path);
        $decoded = json_decode($raw, true);
        $this->config = is_array($decoded) ? $decoded : [];
    }

    private function abs(string $rel): string
    {
        return $this->root . '/' . ltrim($rel, '/');
    }

    /** Build one check entry. */
    private function entry(string $path, string $role, string $status,
                           string $note, string $result): array
    {
        return compact('path', 'role', 'status', 'note', 'result');
    }

    private function checkFile(string $rel, string $status, string $role = '', string $note = ''): array
    {
        $exists = file_exists($this->abs($rel));
        $result = match (true) {
            $exists                  => 'ok',
            $status === 'required'   => 'missing',
            $status === 'bridge'     => 'optional_missing',
            default                  => 'optional_missing',
        };
        return $this->entry($rel, $role, $status, $note, $result);
    }

    private function checkDir(string $dirName, array $cfg): array
    {
        $entries  = [];
        $status   = $cfg['status'] ?? 'optional';
        $role     = $cfg['role']   ?? '';
        $note     = $cfg['note']   ?? '';
        $fullPath = $this->abs($dirName);

        $dirExists = is_dir($fullPath);
        $result = match (true) {
            $dirExists             => 'ok',
            $status === 'required' => 'missing',
            default                => 'optional_missing',
        };
        $entries[] = $this->entry($dirName . '/', $role, $status, $note, $result);

        if (!$dirExists) return $entries;

        // Required files inside the dir
        foreach ($cfg['required_files'] ?? [] as $f) {
            $entries[] = $this->checkFile($dirName . '/' . $f, 'required', $role);
        }

        // Optional files
        foreach ($cfg['optional_files'] ?? [] as $f) {
            $entries[] = $this->checkFile($dirName . '/' . $f, 'optional', $role);
        }

        // Required subdirs
        foreach ($cfg['required_subdirs'] ?? [] as $sub) {
            $subPath = $fullPath . '/' . $sub;
            $r = is_dir($subPath) ? 'ok' : 'missing';
            $entries[] = $this->entry($dirName . '/' . $sub . '/', $role, 'required', '', $r);
        }

        // Optional subdirs
        foreach ($cfg['optional_subdirs'] ?? [] as $sub) {
            $subPath = $fullPath . '/' . $sub;
            $r = is_dir($subPath) ? 'ok' : 'optional_missing';
            $entries[] = $this->entry($dirName . '/' . $sub . '/', $role, 'optional', '', $r);
        }

        // Nested subdirs map (js -> editor, agent, etc.)
        foreach ($cfg['subdirs'] ?? [] as $sub => $subCfg) {
            $subStatus = $subCfg['status'] ?? 'optional';
            $subNote   = $subCfg['note']   ?? '';
            $subPath   = $fullPath . '/' . $sub;
            $r = is_dir($subPath) ? 'ok' : ($subStatus === 'required' ? 'missing' : 'optional_missing');
            $entries[] = $this->entry($dirName . '/' . $sub . '/', $role, $subStatus, $subNote, $r);
        }

        return $entries;
    }

    private function checkBridge(): array
    {
        $entries   = [];
        $bridgeDir = $this->abs('vscode-bridge');
        $bridgeCfg = $this->config['map']['dirs']['vscode-bridge'] ?? [];

        // Already handled by checkDir above for the dir itself; here we
        // validate the specific required bridge files.
        if (!is_dir($bridgeDir)) return $entries;

        foreach ($bridgeCfg['required_files'] ?? [] as $f) {
            $entries[] = $this->checkFile('vscode-bridge/' . $f, 'bridge', 'vscode-sync');
        }

        return $entries;
    }
}

// ------------------------------------------------------------------
// CLI runner
// ------------------------------------------------------------------
if (PHP_SAPI === 'cli' && realpath($argv[0] ?? '') === realpath(__FILE__)) {
    $root   = '';
    $mode   = 'dev';
    $asJson = false;

    for ($i = 1; $i < $argc; $i++) {
        if ($argv[$i] === '--root'  && isset($argv[$i + 1])) { $root = $argv[++$i]; }
        if ($argv[$i] === '--mode'  && isset($argv[$i + 1])) { $mode = $argv[++$i]; }
        if ($argv[$i] === '--json') { $asJson = true; }
    }

    $map    = new FileMap($root, $mode);
    $report = $map->run();

    if ($asJson) {
        echo json_encode($report, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES) . "\n";
    } else {
        echo $map->renderText($report);
        exit($report['summary']['pass'] ? 0 : 1);
    }
}
