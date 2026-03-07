<?php
/**
 * setup.php -- Crissy's Style Tool bootstrap.
 *
 * Drop this file into your project root and open it in a browser.
 * It will:
 *   - Detect the project structure and validate against style-tool.config.json
 *   - Create the config if it does not exist
 *   - Set up the vscode-bridge directory (from vscode-bridge.zip if present,
 *     otherwise by building the scaffold from embedded templates)
 *   - Let you switch between Dev and Build modes
 *   - Show a full file-map health check
 *
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License
 */

declare(strict_types=1);

// ---------------------------------------------------------------------------
// Bootstrap
// ---------------------------------------------------------------------------

define('SETUP_ROOT',   __DIR__);
define('CONFIG_FILE',  SETUP_ROOT . '/style-tool.config.json');
define('FILEMAP_LIB',  SETUP_ROOT . '/scripts/file-map.php');

// Pull in the file-map library if available; inline a minimal version if not.
if (file_exists(FILEMAP_LIB)) {
    require_once FILEMAP_LIB;
} else {
    // Minimal inline stub so setup.php works even before scripts/ is present.
    // The full library is created when you click "Create scripts/file-map.php".
    class FileMap {
        private string $root;
        private array  $config;
        public function __construct(string $root = '') {
            $this->root   = $root ?: SETUP_ROOT;
            $cfg = @file_get_contents($this->root . '/style-tool.config.json');
            $this->config = $cfg ? (json_decode($cfg, true) ?? []) : [];
        }
        public function run(): array {
            $entries = [];
            foreach (['index.php'=>'required','style.css'=>'required','setup.php'=>'optional'] as $f => $s) {
                $exists  = file_exists($this->root . '/' . $f);
                $entries[] = ['path'=>$f,'role'=>'','status'=>$s,'note'=>'','result'=>$exists?'ok':($s==='required'?'missing':'optional_missing')];
            }
            foreach (['data','css','js','vendor','ai','style-sheets','vscode-bridge'] as $d) {
                $exists  = is_dir($this->root . '/' . $d);
                $s = in_array($d,['data','css','js','vendor']) ? 'required' : 'optional';
                $entries[]=['path'=>$d.'/','role'=>$d,'status'=>$s,'note'=>'','result'=>$exists?'ok':($s==='required'?'missing':'optional_missing')];
            }
            $miss = count(array_filter($entries, fn($e)=>$e['result']==='missing'));
            return ['root'=>$this->root,'mode'=>$this->config['mode']??'dev','config_found'=>!empty($this->config),'config_valid'=>!empty($this->config['version']),'entries'=>$entries,'summary'=>['ok'=>count(array_filter($entries,fn($e)=>$e['result']==='ok')),'missing'=>$miss,'optional_missing'=>count(array_filter($entries,fn($e)=>$e['result']==='optional_missing')),'warnings'=>0,'total'=>count($entries),'pass'=>$miss===0]];
        }
        public function renderHtml(array $r): string { return '<pre>'.htmlspecialchars(print_r($r,true)).'</pre>'; }
        public function getConfig(): array { return $this->config; }
        public function getMode(): string  { return $this->config['mode'] ?? 'dev'; }
        public function getRoot(): string  { return $this->root; }
        public function modeConfig(): array{ return $this->config['modes'][$this->getMode()] ?? []; }
        public function featureEnabled(string $f): bool { return (bool)($this->config['feature_flags'][$f] ?? true); }
        public function saveConfig(array $o): bool {
            $this->config = array_replace_recursive($this->config, $o);
            return (bool)file_put_contents($this->root.'/style-tool.config.json', json_encode($this->config,JSON_PRETTY_PRINT|JSON_UNESCAPED_SLASHES)."\n");
        }
    }
}

$map    = new FileMap(SETUP_ROOT);
$action = $_POST['action'] ?? $_GET['action'] ?? '';

// ---------------------------------------------------------------------------
// Default config template (written when none exists)
// ---------------------------------------------------------------------------
function default_config_template(): string {
    $tpl  = file_exists(SETUP_ROOT . '/style-tool.config.json')
          ? file_get_contents(SETUP_ROOT . '/style-tool.config.json')
          : '';
    if ($tpl) return $tpl;

    // Build minimal defaults
    $cfg = [
        '_comment' => 'Crissy\'s Style Tool -- project file structure map.',
        'version'  => '1.0.0',
        'tool'     => 'crissys-style-tool',
        'project'  => ['name'=>'Crissy\'s Style Tool','root'=>'.','entry'=>'index.php','description'=>'Live HTML/CSS/JS editor.'],
        'mode'     => 'dev',
        'map'      => [
            'files' => [
                ['path'=>'index.php',     'role'=>'entry-point', 'status'=>'required'],
                ['path'=>'style.css',     'role'=>'base-styles', 'status'=>'required'],
                ['path'=>'push-pull.php', 'role'=>'cli-sync',    'status'=>'optional'],
            ],
            'dirs' => [
                'data'          => ['status'=>'required','role'=>'php-data',        'required_files'=>['css-properties.php','property-values.php','default-content.php']],
                'css'           => ['status'=>'required','role'=>'css-modules',     'required_files'=>['base.css','layout.css']],
                'js'            => ['status'=>'required','role'=>'js-modules',      'required_files'=>['cdn-loader.js','utils.js','storage.js','editor.js','app.js']],
                'vendor'        => ['status'=>'required','role'=>'third-party',     'required_subdirs'=>['codemirror']],
                'ai'            => ['status'=>'optional','role'=>'ai-backend',      'auto_create'=>false],
                'style-sheets'  => ['status'=>'optional','role'=>'stylesheet-library','auto_create'=>true],
                'vscode-bridge' => ['status'=>'bridge',  'role'=>'vscode-sync',    'auto_create'=>true,'required_subdirs'=>['api','js','data','projects'],'setup_via'=>'setup.php','zip_name'=>'vscode-bridge.zip'],
            ],
        ],
        'modes' => [
            'dev'   => ['debug'=>true, 'source_maps'=>true, 'hot_reload'=>true, 'check_structure'=>true, 'server_port'=>9879, 'display_errors'=>true,  'log_level'=>'verbose','csp_mode'=>'relaxed','enable_ai'=>true,'wasm_enabled'=>false],
            'build' => ['debug'=>false,'source_maps'=>false,'hot_reload'=>false,'check_structure'=>false,'server_port'=>443,  'display_errors'=>false, 'log_level'=>'error',  'csp_mode'=>'strict', 'enable_ai'=>true,'wasm_enabled'=>false,'output_dir'=>'dist','minify_css'=>false,'minify_js'=>false,'exclude_dirs'=>['dev-tools','imgui-browser','dev-browser','debug-tool','c_tools','logs','.git']],
        ],
        'vscode_bridge' => ['api_endpoint'=>'/vscode-bridge/api/projects.php','poll_interval_ms'=>4000,'default_project'=>'crissys-style-tool','projects_dir'=>'vscode-bridge/projects','data_dir'=>'vscode-bridge/data','sqlite_db'=>'vscode-bridge/data/projects.db'],
        'feature_flags' => ['ai_chat'=>true,'agent'=>true,'wireframe'=>true,'color_harmony'=>true,'indent_guides'=>true,'fuzzy_autocomplete'=>true,'vscode_bridge'=>true,'style_sheets'=>true,'theme_randomizer'=>true],
    ];
    return json_encode($cfg, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE) . "\n";
}

// ---------------------------------------------------------------------------
// Actions (POST handlers)
// ---------------------------------------------------------------------------
$actionMsg = '';
$actionOk  = true;

if ($action === 'create_config') {
    if (file_exists(CONFIG_FILE)) {
        $actionMsg = 'style-tool.config.json already exists -- nothing changed.';
    } else {
        $wrote = file_put_contents(CONFIG_FILE, default_config_template());
        if ($wrote) {
            $actionMsg = 'Created style-tool.config.json.';
            $map = new FileMap(SETUP_ROOT); // reload
        } else {
            $actionMsg = 'ERROR: could not write style-tool.config.json. Check directory permissions.';
            $actionOk  = false;
        }
    }
}

if ($action === 'save_mode') {
    $newMode = in_array($_POST['mode'] ?? '', ['dev','build']) ? $_POST['mode'] : 'dev';
    $map->saveConfig(['mode' => $newMode]);
    $actionMsg = 'Mode set to ' . $newMode . '.';
    $map = new FileMap(SETUP_ROOT);
}

if ($action === 'save_feature_flags') {
    $flags = [];
    foreach (['ai_chat','agent','wireframe','color_harmony','indent_guides','fuzzy_autocomplete','vscode_bridge','style_sheets','theme_randomizer'] as $f) {
        $flags[$f] = isset($_POST['ff_' . $f]);
    }
    $map->saveConfig(['feature_flags' => $flags]);
    $actionMsg = 'Feature flags saved.';
    $map = new FileMap(SETUP_ROOT);
}

if ($action === 'save_project_meta') {
    $meta = [
        'project' => [
            'name'        => trim($_POST['project_name'] ?? ''),
            'description' => trim($_POST['project_desc'] ?? ''),
        ],
    ];
    $map->saveConfig($meta);
    $actionMsg = 'Project info saved.';
    $map = new FileMap(SETUP_ROOT);
}

if ($action === 'init_bridge') {
    $result = setup_vscode_bridge(SETUP_ROOT);
    $actionMsg = $result['msg'];
    $actionOk  = $result['ok'];
}

if ($action === 'create_scripts_dir') {
    $dir = SETUP_ROOT . '/scripts';
    if (!is_dir($dir)) mkdir($dir, 0755, true);
    // Tip user: full file-map.php can be fetched via the style tool repo.
    // For now just create a placeholder so future require_once works.
    $placeholder = "<?php\n// scripts/file-map.php\n// This file is created by setup.php.\n// For the full implementation visit the project repo.\n// A full version with FileMap class is generated here when available.\n";
    if (!file_exists(SETUP_ROOT . '/scripts/file-map.php')) {
        file_put_contents(SETUP_ROOT . '/scripts/file-map.php', $placeholder);
        $actionMsg = 'Created scripts/ directory and placeholder file-map.php.';
    } else {
        $actionMsg = 'scripts/file-map.php already exists.';
    }
}

// ---------------------------------------------------------------------------
// VSCode bridge scaffold builder
// ---------------------------------------------------------------------------
function setup_vscode_bridge(string $root): array
{
    $base = $root . '/vscode-bridge';
    $msgs = [];
    $ok   = true;

    // 0. Check for zip first
    $zip = $root . '/vscode-bridge.zip';
    if (file_exists($zip) && class_exists('ZipArchive')) {
        $za = new ZipArchive();
        if ($za->open($zip) === true) {
            $za->extractTo($root);
            $za->close();
            $msgs[] = 'Extracted vscode-bridge.zip.';
            return ['ok' => true, 'msg' => implode(' ', $msgs)];
        }
        $msgs[] = 'Found vscode-bridge.zip but could not open it -- building scaffold instead.';
    }

    // 1. Create directory structure
    $dirs = ['', '/api', '/js', '/data', '/projects', '/projects/custom_design_assets'];
    foreach ($dirs as $d) {
        $full = $base . $d;
        if (!is_dir($full)) {
            if (!mkdir($full, 0755, true)) {
                $msgs[] = 'ERROR: could not create ' . $full;
                $ok = false;
            } else {
                $msgs[] = 'Created ' . 'vscode-bridge' . $d . '/';
            }
        }
    }

    // 2. Data files
    $signal = ['hasUpdate' => false, 'name' => '', 'savedAt' => ''];
    $ack    = ['lastSeen' => ''];
    $dataFiles = [
        '/data/project-update-signal.json' => json_encode($signal, JSON_PRETTY_PRINT),
        '/data/project-update-ack.json'    => json_encode($ack,    JSON_PRETTY_PRINT),
    ];
    foreach ($dataFiles as $rel => $content) {
        $full = $base . $rel;
        if (!file_exists($full)) {
            file_put_contents($full, $content . "\n");
            $msgs[] = 'Created vscode-bridge' . $rel;
        }
    }

    // 3. Starter project files
    $starter = [
        '/projects/html-editor.html' => "<!-- Starter HTML for Crissy's Style Tool -->\n<div class=\"container\">\n  <h1>Hello from the VS Code Bridge</h1>\n  <p>Edit this file in VS Code, then run: php push-pull.php push</p>\n</div>\n",
        '/projects/css-editor.css'   => "/* Starter CSS */\n.container {\n  font-family: sans-serif;\n  max-width: 800px;\n  margin: 2rem auto;\n  padding: 1rem;\n}\n",
        '/projects/js-editor.js'     => "// Starter JS\n",
    ];
    foreach ($starter as $rel => $content) {
        $full = $base . $rel;
        if (!file_exists($full)) {
            file_put_contents($full, $content);
            $msgs[] = 'Created vscode-bridge' . $rel;
        }
    }

    // 4. api/projects.php
    $apiFile = $base . '/api/projects.php';
    if (!file_exists($apiFile)) {
        $content = bridge_api_template();
        file_put_contents($apiFile, $content);
        $msgs[] = 'Created vscode-bridge/api/projects.php';
    }

    // 5. js/bridge-sync.js
    $jsFile = $base . '/js/bridge-sync.js';
    if (!file_exists($jsFile)) {
        $content = bridge_sync_js_template();
        file_put_contents($jsFile, $content);
        $msgs[] = 'Created vscode-bridge/js/bridge-sync.js';
    }

    if (empty($msgs)) $msgs[] = 'VSCode bridge already fully set up -- nothing changed.';

    return ['ok' => $ok, 'msg' => implode('<br>', $msgs)];
}

// ---------------------------------------------------------------------------
// Embedded bridge file templates (used when vscode-bridge does not exist yet)
// ---------------------------------------------------------------------------
function bridge_api_template(): string {
    return <<<'PHP'
<?php
/**
 * vscode-bridge/api/projects.php
 * SQLite-backed project storage generated by setup.php.
 * Full routes: list, get, save, delete, backups, restore, poll_update, ack_update, sync_to_bridge
 */
declare(strict_types=1);
header('Content-Type: application/json; charset=UTF-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') { http_response_code(204); exit; }

define('DATA_DIR',            __DIR__ . '/../data');
define('PROJECTS_DIR',        __DIR__ . '/../projects');
define('DB_PATH',             DATA_DIR . '/projects.db');
define('UPDATE_SIGNAL_FILE',  DATA_DIR . '/project-update-signal.json');
define('UPDATE_ACK_FILE',     DATA_DIR . '/project-update-ack.json');

function ensureDataDir(): void { if (!is_dir(DATA_DIR)) mkdir(DATA_DIR, 0755, true); }
function jsonOut(array $d, int $c=200): void { http_response_code($c); echo json_encode($d, JSON_UNESCAPED_UNICODE|JSON_UNESCAPED_SLASHES); exit; }
function readBody(): array { $r=file_get_contents('php://input'); if(!$r) return []; $d=json_decode($r,true); return is_array($d)?$d:[]; }

function getDB(): PDO {
    ensureDataDir();
    $db = new PDO('sqlite:'.DB_PATH);
    $db->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
    $db->exec("CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL UNIQUE, html TEXT NOT NULL DEFAULT '', css TEXT NOT NULL DEFAULT '', js TEXT NOT NULL DEFAULT '', source TEXT NOT NULL DEFAULT 'browser', created_at TEXT NOT NULL DEFAULT (datetime('now')), updated_at TEXT NOT NULL DEFAULT (datetime('now')))");
    $db->exec("CREATE TABLE IF NOT EXISTS backups  (id INTEGER PRIMARY KEY AUTOINCREMENT, project_name TEXT NOT NULL, html TEXT, css TEXT, js TEXT, backed_up_at TEXT NOT NULL DEFAULT (datetime('now')))");
    return $db;
}

$action = $_GET['action'] ?? $_POST['action'] ?? '';
$name   = trim($_GET['name'] ?? '');

try {
    switch ($action) {
        case 'list':
            $rows = getDB()->query("SELECT name, updated_at FROM projects ORDER BY updated_at DESC")->fetchAll(PDO::FETCH_ASSOC);
            jsonOut(['success'=>true,'projects'=>$rows]);
        case 'get':
            if (!$name) jsonOut(['success'=>false,'error'=>'name required'],400);
            $row = getDB()->prepare("SELECT * FROM projects WHERE name=?")->execute([$name]) ? getDB()->prepare("SELECT * FROM projects WHERE name=?") : null;
            $st = getDB()->prepare("SELECT * FROM projects WHERE name=?"); $st->execute([$name]); $row=$st->fetch(PDO::FETCH_ASSOC);
            if (!$row) jsonOut(['success'=>false,'error'=>'not found'],404);
            jsonOut(['success'=>true,'project'=>$row]);
        case 'save':
            $b=readBody(); $n=trim($b['name']??''); if(!$n) jsonOut(['success'=>false,'error'=>'name required'],400);
            $db=getDB();
            $ex=$db->prepare("SELECT id,html,css,js FROM projects WHERE name=?"); $ex->execute([$n]); $old=$ex->fetch(PDO::FETCH_ASSOC);
            if($old){$db->prepare("INSERT INTO backups(project_name,html,css,js) VALUES(?,?,?,?)")->execute([$n,$old['html'],$old['css'],$old['js']]);}
            $db->prepare("INSERT INTO projects(name,html,css,js,source,updated_at) VALUES(?,?,?,?,?,datetime('now')) ON CONFLICT(name) DO UPDATE SET html=excluded.html,css=excluded.css,js=excluded.js,source=excluded.source,updated_at=excluded.updated_at")->execute([$n,$b['html']??'',$b['css']??'',$b['js']??'',$b['source']??'browser']);
            $sig=['hasUpdate'=>true,'name'=>$n,'savedAt'=>date('c'),'source'=>$b['source']??'browser'];
            file_put_contents(UPDATE_SIGNAL_FILE, json_encode($sig));
            jsonOut(['success'=>true,'name'=>$n]);
        case 'poll_update':
            $sig=@json_decode(@file_get_contents(UPDATE_SIGNAL_FILE),true)??[];
            $ack=@json_decode(@file_get_contents(UPDATE_ACK_FILE),true)??[];
            $has=($sig['hasUpdate']??false) && ($sig['savedAt']??'')!==($ack['lastSeen']??'');
            if($has){$row=null;$n=$sig['name']??'';if($n){$st=getDB()->prepare("SELECT html,css,js FROM projects WHERE name=?");$st->execute([$n]);$row=$st->fetch(PDO::FETCH_ASSOC);}jsonOut(['success'=>true,'hasUpdate'=>true,'name'=>$n,'source'=>$sig['source']??'unknown']+($row??[]));}
            jsonOut(['success'=>true,'hasUpdate'=>false]);
        case 'ack_update':
            $sig=@json_decode(@file_get_contents(UPDATE_SIGNAL_FILE),true)??[];
            file_put_contents(UPDATE_ACK_FILE, json_encode(['lastSeen'=>$sig['savedAt']??'']));
            jsonOut(['success'=>true]);
        case 'delete':
            if(!$name) jsonOut(['success'=>false,'error'=>'name required'],400);
            getDB()->prepare("DELETE FROM projects WHERE name=?")->execute([$name]);
            jsonOut(['success'=>true]);
        default:
            jsonOut(['success'=>false,'error'=>'unknown action'],400);
    }
} catch(Throwable $e) {
    jsonOut(['success'=>false,'error'=>$e->getMessage()],500);
}
PHP;
}

function bridge_sync_js_template(): string {
    return <<<'JS'
/**
 * vscode-bridge/js/bridge-sync.js -- generated by setup.php
 * Two-way sync between Crissy's Style Tool and the VS Code bridge.
 * Generated stub -- replace with the full implementation from the project repo if available.
 */
(function (global) {
    'use strict';
    var PROJECTS_API   = '/vscode-bridge/api/projects.php';
    var POLL_MS        = 4000;
    var STORAGE_KEY    = 'bridgeSync_enabled';
    var _enabled = false, _timer = null;

    function log(m) { console.log('[BridgeSync] ' + m); }

    function poll() {
        if (!_enabled) return;
        fetch(PROJECTS_API + '?action=poll_update')
            .then(function (r) { return r.json(); })
            .then(function (d) {
                if (!d.hasUpdate) return;
                var name = d.name || '(unnamed)';
                log('Update from bridge: ' + name);
                if (global.LiveCSS && global.LiveCSS.storage &&
                    typeof global.LiveCSS.storage.pollProjectUpdate === 'function') {
                    global.LiveCSS.storage.pollProjectUpdate();
                }
            })
            .catch(function (e) { console.warn('[BridgeSync] poll error', e); });
    }

    function setEnabled(on) {
        _enabled = on;
        localStorage.setItem(STORAGE_KEY, on ? '1' : '0');
        if (on) { _timer = setInterval(poll, POLL_MS); log('enabled'); }
        else    { clearInterval(_timer); _timer = null; log('disabled'); }
        var btn = document.getElementById('vscodeBridgeToggle');
        if (btn) {
            btn.setAttribute('aria-pressed', on ? 'true' : 'false');
            btn.classList.toggle('btn-active', on);
        }
        var help = document.getElementById('helpBtn');
        if (help) help.style.display = on ? '' : 'none';
    }

    function init() {
        var btn = document.getElementById('vscodeBridgeToggle');
        if (!btn) return;
        var saved = localStorage.getItem(STORAGE_KEY);
        setEnabled(saved === '1');
        btn.addEventListener('click', function () { setEnabled(!_enabled); });
    }

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
}(window));
JS;
}

// ---------------------------------------------------------------------------
// Re-run report after actions
// ---------------------------------------------------------------------------
$report = $map->run();
$config = $map->getConfig();
$mode   = $map->getMode();
$mCfg   = $map->modeConfig();
$projName = $config['project']['name'] ?? "Crissy's Style Tool";
$projDesc = $config['project']['description'] ?? '';
$activeTab = $_GET['tab'] ?? 'structure';

// Feature flags
$flags = $config['feature_flags'] ?? [];
$featureList = [
    'ai_chat'           => 'AI Chat',
    'agent'             => 'Agent',
    'wireframe'         => 'Wireframe tool',
    'color_harmony'     => 'Color Harmony',
    'indent_guides'     => 'Indent Guides',
    'fuzzy_autocomplete'=> 'Fuzzy Autocomplete',
    'vscode_bridge'     => 'VS Code Bridge',
    'style_sheets'      => 'Style Sheets Library',
    'theme_randomizer'  => 'Theme Randomizer',
];

// Bridge status
$bridgeOk = is_dir(SETUP_ROOT . '/vscode-bridge/api')
         && file_exists(SETUP_ROOT . '/vscode-bridge/js/bridge-sync.js')
         && file_exists(SETUP_ROOT . '/vscode-bridge/api/projects.php');

// Build mode info
$buildCfg   = $config['modes']['build'] ?? [];
$excludeDirs = $buildCfg['exclude_dirs'] ?? [];
?>
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Setup -- <?= htmlspecialchars($projName) ?></title>
<style>
:root {
    --bg:       #080715;
    --surface:  #0f0d1e;
    --raised:   #181530;
    --border:   rgba(99,102,241,0.18);
    --accent:   #6366f1;
    --accent2:  rgba(99,102,241,0.25);
    --text:     #e6e9f5;
    --dim:      #666e80;
    --ok:       #34d399;
    --miss:     #f87171;
    --opt:      #a78bfa;
    --warn:     #fbbf24;
    --radius:   8px;
    --mono:     'JetBrains Mono','Fira Code',monospace;
}
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body  { background: var(--bg); color: var(--text); font-family: system-ui,sans-serif; font-size: 14px; min-height: 100vh; }
a     { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
code  { font-family: var(--mono); font-size: 12px; background: rgba(99,102,241,0.1); padding: 1px 5px; border-radius: 3px; }
pre   { font-family: var(--mono); font-size: 12px; }

/* Shell */
.shell   { display: flex; flex-direction: column; min-height: 100vh; }
.topbar  { background: var(--surface); border-bottom: 1px solid var(--border); padding: 14px 28px; display: flex; align-items: center; gap: 16px; }
.topbar h1 { font-size: 16px; font-weight: 600; color: var(--text); }
.topbar .mode-badge { font-size: 11px; padding: 3px 10px; border-radius: 12px; font-weight: 600; letter-spacing: .05em; border: 1px solid var(--accent); color: var(--accent); background: var(--accent2); }
.topbar .mode-badge.build { border-color: var(--warn); color: var(--warn); background: rgba(251,191,36,.15); }
.topbar .root-hint { font-size: 11px; color: var(--dim); font-family: var(--mono); margin-left: auto; }

.content { display: flex; flex: 1; }

/* Sidebar */
.sidebar { width: 200px; flex-shrink: 0; background: var(--surface); border-right: 1px solid var(--border); padding: 20px 0; }
.sidebar-item { display: block; padding: 9px 20px; color: var(--dim); font-size: 13px; cursor: pointer; border: none; background: none; width: 100%; text-align: left; border-left: 3px solid transparent; transition: color .15s, border-color .15s, background .15s; }
.sidebar-item:hover { color: var(--text); background: rgba(99,102,241,.07); }
.sidebar-item.active { color: var(--accent); border-left-color: var(--accent); background: rgba(99,102,241,.1); }

/* Main area */
.main { flex: 1; padding: 28px 32px; overflow-y: auto; }
.section { background: var(--surface); border: 1px solid var(--border); border-radius: var(--radius); padding: 22px 24px; margin-bottom: 20px; }
.section h2 { font-size: 14px; font-weight: 600; color: var(--accent); margin-bottom: 16px; letter-spacing: .04em; text-transform: uppercase; }
.section h3 { font-size: 13px; font-weight: 600; color: var(--text); margin: 16px 0 8px; }

/* Status message */
.msg { padding: 10px 16px; border-radius: var(--radius); font-size: 13px; margin-bottom: 18px; }
.msg.ok   { background: rgba(52,211,153,.12); border: 1px solid rgba(52,211,153,.35); color: var(--ok); }
.msg.err  { background: rgba(248,113,113,.12); border: 1px solid rgba(248,113,113,.35); color: var(--miss); }

/* Buttons */
.btn { display: inline-flex; align-items: center; gap: 6px; padding: 8px 18px; border-radius: var(--radius); font-size: 13px; font-family: inherit; cursor: pointer; border: 1px solid var(--accent); background: var(--accent2); color: var(--accent); font-weight: 500; transition: background .12s, color .12s; }
.btn:hover { background: var(--accent); color: #fff; }
.btn.btn-primary { background: var(--accent); color: #fff; }
.btn.btn-primary:hover { background: #5254cc; }
.btn.btn-warn { border-color: var(--warn); background: rgba(251,191,36,.12); color: var(--warn); }
.btn.btn-warn:hover { background: var(--warn); color: #000; }
.btn.btn-ok { border-color: var(--ok); background: rgba(52,211,153,.12); color: var(--ok); }
.btn.btn-ok:hover { background: var(--ok); color: #000; }
.btn-row { display: flex; gap: 10px; flex-wrap: wrap; margin-top: 14px; }
button[type=submit].btn { cursor: pointer; }

/* File map table */
.fm-table { width: 100%; border-collapse: collapse; font-size: 12px; font-family: var(--mono); margin-top: 10px; }
.fm-table th { text-align: left; padding: 6px 10px; color: var(--dim); font-weight: 600; border-bottom: 1px solid var(--border); font-size: 11px; letter-spacing: .06em; text-transform: uppercase; }
.fm-table td { padding: 5px 10px; border-bottom: 1px solid rgba(99,102,241,.06); vertical-align: middle; }
.fm-ok      td:first-child { color: var(--ok); }
.fm-missing td:first-child { color: var(--miss); font-weight: 700; }
.fm-optional td:first-child { color: var(--opt); }
.fm-warn    td:first-child { color: var(--warn); }
.fm-summary { margin-top: 12px; font-size: 12px; color: var(--dim); padding: 8px 10px; border-radius: 4px; background: rgba(99,102,241,.06); }
.fm-pass { color: var(--ok); }
.fm-fail { color: var(--miss); }
.fm-root,.fm-mode { font-size: 12px; color: var(--dim); margin-bottom: 6px; }

/* Mode switcher */
.mode-switch { display: flex; gap: 10px; align-items: center; }
.mode-pill { padding: 6px 18px; border-radius: 20px; border: 1px solid var(--border); color: var(--dim); cursor: pointer; font-size: 12px; font-weight: 600; letter-spacing: .05em; text-transform: uppercase; background: var(--raised); }
.mode-pill.dev-active  { border-color: var(--accent); color: var(--accent); background: var(--accent2); }
.mode-pill.build-active { border-color: var(--warn); color: var(--warn); background: rgba(251,191,36,.1); }

/* Feature flags grid */
.ff-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(200px, 1fr)); gap: 8px; margin-top: 10px; }
.ff-item { display: flex; align-items: center; gap: 8px; padding: 8px 12px; border-radius: 6px; background: var(--raised); border: 1px solid var(--border); }
.ff-item label { font-size: 12px; color: var(--text); cursor: pointer; user-select: none; }
input[type=checkbox] { accent-color: var(--accent); width: 14px; height: 14px; cursor: pointer; }

/* Form rows */
.form-row { display: flex; flex-direction: column; gap: 4px; margin-bottom: 14px; }
.form-row label { font-size: 11px; color: var(--dim); letter-spacing: .05em; text-transform: uppercase; }
.form-control { background: var(--bg); border: 1px solid var(--border); color: var(--text); padding: 8px 12px; border-radius: 6px; font-family: inherit; font-size: 13px; outline: none; width: 100%; }
.form-control:focus { border-color: var(--accent); }
.form-control.mono { font-family: var(--mono); font-size: 12px; }

/* Build dir list */
.exclude-list { list-style: none; margin-top: 8px; }
.exclude-list li { font-family: var(--mono); font-size: 12px; color: var(--dim); padding: 3px 0; }
.exclude-list li::before { content: '-- '; color: var(--miss); }

/* Bridge status */
.bridge-status { display: flex; align-items: center; gap: 10px; margin-bottom: 14px; }
.dot { width: 10px; height: 10px; border-radius: 50%; flex-shrink: 0; }
.dot.ok   { background: var(--ok); box-shadow: 0 0 6px var(--ok); }
.dot.miss { background: var(--miss); box-shadow: 0 0 6px var(--miss); }

/* Config JSON editor */
.json-editor { width: 100%; min-height: 340px; background: var(--bg); border: 1px solid var(--border); color: #c7c7f0; font-family: var(--mono); font-size: 11px; padding: 14px; border-radius: 6px; resize: vertical; outline: none; line-height: 1.6; }
.json-editor:focus { border-color: var(--accent); }

/* Mode config grid */
.cfg-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 8px 24px; margin-top: 10px; }
.cfg-row  { display: flex; flex-direction: column; gap: 3px; }
.cfg-key  { font-size: 11px; color: var(--dim); font-family: var(--mono); }
.cfg-val  { font-size: 12px; color: var(--text); font-family: var(--mono); padding: 4px 8px; background: var(--raised); border-radius: 4px; border: 1px solid var(--border); }
</style>
</head>
<body>
<div class="shell">

<div class="topbar">
    <h1><?= htmlspecialchars($projName) ?> &mdash; Setup</h1>
    <span class="mode-badge <?= $mode === 'build' ? 'build' : '' ?>"><?= htmlspecialchars(strtoupper($mode)) ?></span>
    <span class="root-hint"><?= htmlspecialchars(SETUP_ROOT) ?></span>
</div>

<div class="content">
<nav class="sidebar">
    <?php
    $tabs = [
        'structure'  => 'File Structure',
        'mode'       => 'Mode / Settings',
        'features'   => 'Feature Flags',
        'bridge'     => 'VS Code Bridge',
        'build'      => 'Build Mode',
        'config'     => 'Config JSON',
    ];
    foreach ($tabs as $id => $label): ?>
    <a href="?tab=<?= $id ?>" class="sidebar-item <?= $activeTab === $id ? 'active' : '' ?>"><?= $label ?></a>
    <?php endforeach; ?>
</nav>

<main class="main">

<?php if ($actionMsg): ?>
<div class="msg <?= $actionOk ? 'ok' : 'err' ?>"><?= $actionMsg ?></div>
<?php endif; ?>

<?php if (!file_exists(CONFIG_FILE)): ?>
<div class="section">
    <h2>Config not found</h2>
    <p style="color:var(--dim);margin-bottom:14px">style-tool.config.json is missing. Create it to configure file structure mapping, modes, and feature flags.</p>
    <form method="post">
        <input type="hidden" name="action" value="create_config">
        <button type="submit" class="btn btn-primary">Create style-tool.config.json</button>
    </form>
</div>
<?php endif; ?>

<!-- ================================================================ STRUCTURE -->
<?php if ($activeTab === 'structure'): ?>
<div class="section">
    <h2>File Structure Map</h2>
    <p style="color:var(--dim);font-size:12px;margin-bottom:14px">
        Validates the project root against the map declared in style-tool.config.json.
        Green rows are present. Red rows are required files that are missing.
        Purple rows are optional and not present.
    </p>
    <?= $map->renderHtml($report) ?>
    <?php if (!file_exists(FILEMAP_LIB)): ?>
    <div class="btn-row">
        <form method="post">
            <input type="hidden" name="action" value="create_scripts_dir">
            <button type="submit" class="btn">Create scripts/ directory</button>
        </form>
    </div>
    <p style="color:var(--dim);font-size:11px;margin-top:8px">The full scripts/file-map.php validator will give a more detailed report once created.</p>
    <?php endif; ?>
</div>

<!-- ================================================================ MODE -->
<?php elseif ($activeTab === 'mode'): ?>
<div class="section">
    <h2>Runtime Mode</h2>
    <p style="color:var(--dim);font-size:12px;margin-bottom:16px">
        Dev mode has debug output, hot-reload hints, and relaxed CSP.
        Build mode disables debug output, enforces strict CSP, and sets production flags.
        The active mode is saved to style-tool.config.json.
    </p>
    <form method="post">
        <input type="hidden" name="action" value="save_mode">
        <div class="mode-switch">
            <label class="mode-pill <?= $mode === 'dev' ? 'dev-active' : '' ?>">
                <input type="radio" name="mode" value="dev"   <?= $mode === 'dev'   ? 'checked' : '' ?> onchange="this.form.submit()" style="display:none"> Dev
            </label>
            <label class="mode-pill <?= $mode === 'build' ? 'build-active' : '' ?>">
                <input type="radio" name="mode" value="build" <?= $mode === 'build' ? 'checked' : '' ?> onchange="this.form.submit()" style="display:none"> Build
            </label>
        </div>
    </form>

    <h3>Active mode settings <code><?= htmlspecialchars($mode) ?></code></h3>
    <div class="cfg-grid">
        <?php foreach ($mCfg as $k => $v): if (is_array($v)) continue; ?>
        <div class="cfg-row">
            <span class="cfg-key"><?= htmlspecialchars($k) ?></span>
            <span class="cfg-val"><?= htmlspecialchars(is_bool($v) ? ($v ? 'true' : 'false') : (string)$v) ?></span>
        </div>
        <?php endforeach; ?>
    </div>
</div>

<div class="section">
    <h2>Project Info</h2>
    <form method="post">
        <input type="hidden" name="action" value="save_project_meta">
        <div class="form-row">
            <label>Project Name</label>
            <input type="text" name="project_name" class="form-control" value="<?= htmlspecialchars($projName) ?>">
        </div>
        <div class="form-row">
            <label>Description</label>
            <input type="text" name="project_desc" class="form-control" value="<?= htmlspecialchars($projDesc) ?>">
        </div>
        <button type="submit" class="btn btn-primary">Save</button>
    </form>
</div>

<!-- ================================================================ FEATURES -->
<?php elseif ($activeTab === 'features'): ?>
<div class="section">
    <h2>Feature Flags</h2>
    <p style="color:var(--dim);font-size:12px;margin-bottom:12px">
        Toggle features on or off. Disabled features still have their files present on disk;
        the flags are read by index.php and the JS modules to suppress the UI.
        This lets you run the style tool without AI, without the agent, etc.
    </p>
    <form method="post">
        <input type="hidden" name="action" value="save_feature_flags">
        <div class="ff-grid">
            <?php foreach ($featureList as $key => $label): ?>
            <div class="ff-item">
                <input type="checkbox" name="ff_<?= $key ?>" id="ff_<?= $key ?>"
                    <?= ($flags[$key] ?? true) ? 'checked' : '' ?>>
                <label for="ff_<?= $key ?>"><?= htmlspecialchars($label) ?></label>
            </div>
            <?php endforeach; ?>
        </div>
        <div class="btn-row"><button type="submit" class="btn btn-primary">Save Feature Flags</button></div>
    </form>
</div>

<!-- ================================================================ BRIDGE -->
<?php elseif ($activeTab === 'bridge'): ?>
<div class="section">
    <h2>VS Code Bridge</h2>

    <div class="bridge-status">
        <span class="dot <?= $bridgeOk ? 'ok' : 'miss' ?>"></span>
        <span style="font-size:13px"><?= $bridgeOk ? 'Bridge is set up and ready.' : 'Bridge not set up -- click Initialize to create it.' ?></span>
    </div>

    <p style="color:var(--dim);font-size:12px;line-height:1.6;margin-bottom:16px">
        The VS Code Bridge lets GitHub Copilot edit your HTML, CSS, and JS files directly.
        It creates a <code>vscode-bridge/</code> directory with a SQLite database, a polling
        API, and the JavaScript client that the tool uses for two-way sync.
        <br><br>
        If you have <code>vscode-bridge.zip</code> in the project root it will be extracted.
        Otherwise the scaffold is built from embedded templates.
    </p>

    <?php if (file_exists(SETUP_ROOT . '/vscode-bridge.zip')): ?>
    <p style="color:var(--ok);font-size:12px;margin-bottom:12px">vscode-bridge.zip found -- will be extracted on initialize.</p>
    <?php endif; ?>

    <form method="post">
        <input type="hidden" name="action" value="init_bridge">
        <button type="submit" class="btn <?= $bridgeOk ? 'btn-ok' : 'btn-primary' ?>">
            <?= $bridgeOk ? 'Re-initialize Bridge' : 'Initialize VS Code Bridge' ?>
        </button>
    </form>

    <?php if ($bridgeOk): ?>
    <h3>Bridge file map</h3>
    <table class="fm-table">
        <tr><th>Status</th><th>Path</th></tr>
        <?php
        $bridgeFiles = [
            'vscode-bridge/api/projects.php',
            'vscode-bridge/js/bridge-sync.js',
            'vscode-bridge/data/project-update-signal.json',
            'vscode-bridge/data/project-update-ack.json',
            'vscode-bridge/projects/html-editor.html',
            'vscode-bridge/projects/css-editor.css',
            'vscode-bridge/projects/js-editor.js',
        ];
        foreach ($bridgeFiles as $f):
            $exists = file_exists(SETUP_ROOT . '/' . $f);
        ?>
        <tr class="<?= $exists ? 'fm-ok' : 'fm-optional' ?>">
            <td><?= $exists ? '&#10003;' : '&#8212;' ?></td>
            <td><code><?= htmlspecialchars($f) ?></code></td>
        </tr>
        <?php endforeach; ?>
    </table>

    <h3>Usage</h3>
    <pre style="color:var(--dim);line-height:1.7;background:var(--raised);padding:12px 14px;border-radius:6px;margin-top:6px;font-size:11px">
# Push current editor state to the bridge
php push-pull.php push

# Pull a project from the bridge back into the editor
php push-pull.php pull

# List saved projects
php push-pull.php list

# From VS Code: Copilot edits vscode-bridge/projects/ files then runs:
php push-pull.php push my-project
    </pre>
    <?php endif; ?>
</div>

<div class="section">
    <h2>Bridge Config</h2>
    <div class="cfg-grid">
        <?php foreach ($config['vscode_bridge'] ?? [] as $k => $v): ?>
        <div class="cfg-row">
            <span class="cfg-key"><?= htmlspecialchars($k) ?></span>
            <span class="cfg-val"><?= htmlspecialchars((string)$v) ?></span>
        </div>
        <?php endforeach; ?>
    </div>
    <p style="color:var(--dim);font-size:11px;margin-top:10px">Edit these values in the Config JSON tab.</p>
</div>

<!-- ================================================================ BUILD -->
<?php elseif ($activeTab === 'build'): ?>
<div class="section">
    <h2>Build Mode</h2>
    <p style="color:var(--dim);font-size:12px;line-height:1.6;margin-bottom:16px">
        Build mode is a flag that switches the tool into production configuration.
        Switch to it (Mode tab) before deploying. It disables debug output, sets
        <code>display_errors</code> off, tightens CSP, and declares which directories
        to exclude from any bundle or deploy step.
        <br><br>
        The tool does not run a bundler or minifier automatically because PHP apps
        are typically served directly. The build config is consumed by custom deploy
        scripts or CI pipelines that read style-tool.config.json.
    </p>

    <h3>Current build settings</h3>
    <div class="cfg-grid">
        <?php foreach ($buildCfg as $k => $v): if (is_array($v)) continue; ?>
        <div class="cfg-row">
            <span class="cfg-key"><?= htmlspecialchars($k) ?></span>
            <span class="cfg-val"><?= htmlspecialchars(is_bool($v) ? ($v?'true':'false') : (string)$v) ?></span>
        </div>
        <?php endforeach; ?>
    </div>

    <h3>Excluded from build output</h3>
    <ul class="exclude-list">
        <?php foreach ($excludeDirs as $d): ?>
        <li><?= htmlspecialchars($d) ?></li>
        <?php endforeach; ?>
    </ul>
    <p style="color:var(--dim);font-size:11px;margin-top:8px">Edit the <code>modes.build.exclude_dirs</code> array in the Config JSON tab to change this list.</p>
</div>

<div class="section">
    <h2>Deployment checklist</h2>
    <?php
    $checks = [
        'style-tool.config.json exists'        => file_exists(CONFIG_FILE),
        'index.php exists'                      => file_exists(SETUP_ROOT . '/index.php'),
        'data/ directory present'               => is_dir(SETUP_ROOT . '/data'),
        'vendor/codemirror/ present'            => is_dir(SETUP_ROOT . '/vendor/codemirror'),
        'vscode-bridge set up'                  => $bridgeOk,
        'Current mode is build'                 => $mode === 'build',
        'display_errors is false (build cfg)'   => !($buildCfg['display_errors'] ?? true),
    ];
    ?>
    <table class="fm-table" style="margin-top:6px">
        <tr><th>Status</th><th>Check</th></tr>
        <?php foreach ($checks as $label => $pass): ?>
        <tr class="<?= $pass ? 'fm-ok' : 'fm-optional' ?>">
            <td><?= $pass ? '&#10003;' : '&#8212;' ?></td>
            <td><?= htmlspecialchars($label) ?></td>
        </tr>
        <?php endforeach; ?>
    </table>
</div>

<!-- ================================================================ CONFIG JSON -->
<?php elseif ($activeTab === 'config'): ?>
<div class="section">
    <h2>Config JSON</h2>
    <p style="color:var(--dim);font-size:12px;margin-bottom:14px">
        This is the raw content of <code>style-tool.config.json</code>.
        Edit it directly here and click Save, or edit the file on disk.
        The file is re-read on every page load.
    </p>
    <?php if (file_exists(CONFIG_FILE)): ?>
    <form method="post" action="?tab=config">
        <input type="hidden" name="action" value="save_raw_config">
        <textarea class="json-editor" name="raw_config" spellcheck="false"><?= htmlspecialchars(file_get_contents(CONFIG_FILE)) ?></textarea>
        <div class="btn-row">
            <button type="submit" class="btn btn-primary" onclick="return validateJson(this.form.raw_config)">Save Config</button>
            <a href="?tab=structure" class="btn">Re-run structure check</a>
        </div>
    </form>
    <?php else: ?>
    <p style="color:var(--miss)">style-tool.config.json does not exist. Create it via the Structure tab.</p>
    <?php endif; ?>
</div>
<?php endif; ?>

</main>
</div>
</div>

<?php
// Handle raw config save (done after HTML output to keep the response simple).
if ($action === 'save_raw_config' && isset($_POST['raw_config'])) {
    $raw     = $_POST['raw_config'];
    $decoded = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        // Redirect back with error -- reachable if JS validation was bypassed
        header('Location: ?tab=config&err=invalid_json');
        exit;
    }
    $pretty = json_encode($decoded, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES | JSON_UNESCAPED_UNICODE);
    file_put_contents(CONFIG_FILE, $pretty . "\n");
    header('Location: ?tab=config&saved=1');
    exit;
}
?>

<script>
function validateJson(textarea) {
    try {
        JSON.parse(textarea.value);
        return true;
    } catch (e) {
        alert('Invalid JSON: ' + e.message);
        return false;
    }
}
// Highlight saved/error states from redirect
(function () {
    var p  = new URLSearchParams(location.search);
    var el = document.querySelector('.main');
    if (!el) return;
    if (p.get('saved') === '1') {
        var msg = document.createElement('div');
        msg.className = 'msg ok';
        msg.textContent = 'Config saved.';
        el.insertBefore(msg, el.firstChild);
    }
    if (p.get('err') === 'invalid_json') {
        var msg = document.createElement('div');
        msg.className = 'msg err';
        msg.textContent = 'Invalid JSON -- config not saved.';
        el.insertBefore(msg, el.firstChild);
    }
}());
</script>
</body>
</html>
