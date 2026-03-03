<?php
/**
 * vscode-bridge/api/projects.php
 * SQLite-backed project storage for Crissy's Style Tool.
 *
 * This is the persistent backend for saved projects (HTML + CSS + JS).
 * Works alongside localStorage -- the app can save/load from either, but
 * this gives Copilot (via MCP) a way to write projects that the user can
 * click "Load" to pick up, with automatic backups.
 *
 * Routes:
 *   GET  ?action=list                  -- list all saved project names + timestamps
 *   GET  ?action=get&name=...          -- read a single project (html, css, js)
 *   POST ?action=save                  -- save/overwrite a project (auto-backs up previous)
 *   POST ?action=delete&name=...       -- delete a project
 *   GET  ?action=backups&name=...      -- list backups for a project
 *   GET  ?action=restore&name=...&ts=  -- restore a backup to the active slot
 *   GET  ?action=poll_update           -- browser polls: was a project saved since last check?
 *   POST ?action=ack_update            -- browser acknowledges it saw the latest save
 *
 * All errors log to error_log() and return JSON with success:false so the
 * browser/MCP can console.error them.
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

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------
// Data directory is always relative to this file's location.
// __DIR__ = vscode-bridge/api  ->  __DIR__/../data = vscode-bridge/data
define('DATA_DIR', __DIR__ . '/../data');
define('DB_PATH',           DATA_DIR . '/projects.db');
define('UPDATE_SIGNAL_FILE', DATA_DIR . '/project-update-signal.json');
define('UPDATE_ACK_FILE',    DATA_DIR . '/project-update-ack.json');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
function ensureDataDir(): void {
    if (!is_dir(DATA_DIR)) {
        if (!mkdir(DATA_DIR, 0755, true)) {
            error_log('[Projects] Could not create data dir: ' . DATA_DIR);
        }
    }
}

function jsonOut(array $data, int $code = 200): void {
    http_response_code($code);
    echo json_encode($data, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function readBody(): array {
    $raw = file_get_contents('php://input');
    if (empty($raw)) return [];
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) {
        error_log('[Projects] readBody: invalid JSON - ' . json_last_error_msg());
        return [];
    }
    return $data ?? [];
}

function readJsonFile(string $path): ?array {
    if (!file_exists($path)) return null;
    $raw = file_get_contents($path);
    if ($raw === false) return null;
    $data = json_decode($raw, true);
    if (json_last_error() !== JSON_ERROR_NONE) return null;
    return $data;
}

function writeJsonFile(string $path, array $data): bool {
    ensureDataDir();
    $json = json_encode($data, JSON_UNESCAPED_UNICODE | JSON_PRETTY_PRINT);
    return file_put_contents($path, $json, LOCK_EX) !== false;
}

// ---------------------------------------------------------------------------
// Database
// ---------------------------------------------------------------------------
function getDb(): SQLite3 {
    ensureDataDir();
    $db = new SQLite3(DB_PATH);
    $db->busyTimeout(3000);
    $db->exec('PRAGMA journal_mode=WAL');
    $db->exec('PRAGMA foreign_keys=ON');

    // Main projects table
    $db->exec('
        CREATE TABLE IF NOT EXISTS projects (
            name       TEXT PRIMARY KEY,
            html       TEXT NOT NULL DEFAULT "",
            css        TEXT NOT NULL DEFAULT "",
            js         TEXT NOT NULL DEFAULT "",
            source     TEXT NOT NULL DEFAULT "browser",
            updated_at TEXT NOT NULL
        )
    ');

    // Backups table -- one row per overwrite
    $db->exec('
        CREATE TABLE IF NOT EXISTS project_backups (
            id         INTEGER PRIMARY KEY AUTOINCREMENT,
            name       TEXT NOT NULL,
            html       TEXT NOT NULL DEFAULT "",
            css        TEXT NOT NULL DEFAULT "",
            js         TEXT NOT NULL DEFAULT "",
            source     TEXT NOT NULL DEFAULT "browser",
            backed_up_at TEXT NOT NULL
        )
    ');
    $db->exec('CREATE INDEX IF NOT EXISTS idx_backups_name ON project_backups(name)');

    return $db;
}

// ---------------------------------------------------------------------------
// Router
// ---------------------------------------------------------------------------
try {
    $action = $_GET['action'] ?? '';

    // -- GET ?action=list
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'list') {
        $db   = getDb();
        $stmt = $db->query('SELECT name, source, updated_at, length(html) as html_len, length(css) as css_len, length(js) as js_len FROM projects ORDER BY name');
        $rows = [];
        while ($row = $stmt->fetchArray(SQLITE3_ASSOC)) {
            $rows[] = $row;
        }
        $db->close();
        jsonOut(['success' => true, 'projects' => $rows]);
    }

    // -- GET ?action=get&name=...
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'get') {
        $name = $_GET['name'] ?? '';
        if ($name === '') jsonOut(['success' => false, 'error' => 'Missing name parameter'], 400);

        $db   = getDb();
        $stmt = $db->prepare('SELECT * FROM projects WHERE name = :name');
        $stmt->bindValue(':name', $name, SQLITE3_TEXT);
        $row  = $stmt->execute()->fetchArray(SQLITE3_ASSOC);
        $db->close();

        if (!$row) {
            jsonOut(['success' => false, 'error' => 'Project not found: ' . $name], 404);
        }
        jsonOut(['success' => true, 'project' => $row]);
    }

    // -- POST ?action=save
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'save') {
        $body = readBody();
        $name = $body['name'] ?? '';
        if ($name === '') jsonOut(['success' => false, 'error' => 'Missing project name'], 400);

        $html   = $body['html']   ?? '';
        $css    = $body['css']    ?? '';
        $js     = $body['js']     ?? '';
        $source = $body['source'] ?? 'browser';
        $now    = date('Y-m-d H:i:s');

        $db = getDb();

        // Auto-backup: if the project already exists, copy it to backups first
        $existing = $db->prepare('SELECT * FROM projects WHERE name = :name');
        $existing->bindValue(':name', $name, SQLITE3_TEXT);
        $old = $existing->execute()->fetchArray(SQLITE3_ASSOC);

        if ($old) {
            $bk = $db->prepare('INSERT INTO project_backups (name, html, css, js, source, backed_up_at) VALUES (:name, :html, :css, :js, :source, :ts)');
            $bk->bindValue(':name',   $old['name'],       SQLITE3_TEXT);
            $bk->bindValue(':html',   $old['html'],       SQLITE3_TEXT);
            $bk->bindValue(':css',    $old['css'],        SQLITE3_TEXT);
            $bk->bindValue(':js',     $old['js'],         SQLITE3_TEXT);
            $bk->bindValue(':source', $old['source'],     SQLITE3_TEXT);
            $bk->bindValue(':ts',     $now,               SQLITE3_TEXT);
            $bk->execute();
            error_log('[Projects] Backed up previous version of "' . $name . '"');
        }

        // Upsert the project
        $ins = $db->prepare('INSERT OR REPLACE INTO projects (name, html, css, js, source, updated_at) VALUES (:name, :html, :css, :js, :source, :ts)');
        $ins->bindValue(':name',   $name,   SQLITE3_TEXT);
        $ins->bindValue(':html',   $html,   SQLITE3_TEXT);
        $ins->bindValue(':css',    $css,    SQLITE3_TEXT);
        $ins->bindValue(':js',     $js,     SQLITE3_TEXT);
        $ins->bindValue(':source', $source, SQLITE3_TEXT);
        $ins->bindValue(':ts',     $now,    SQLITE3_TEXT);
        $ins->execute();
        $db->close();

        // Write a signal file so the browser can detect Copilot wrote a project
        writeJsonFile(UPDATE_SIGNAL_FILE, [
            'name'      => $name,
            'source'    => $source,
            'updatedAt' => $now,
        ]);

        error_log('[Projects] Saved "' . $name . '" (' . strlen($html) . ' html, ' . strlen($css) . ' css, ' . strlen($js) . ' js) from ' . $source);
        jsonOut(['success' => true, 'name' => $name, 'updatedAt' => $now, 'backedUp' => $old !== false]);
    }

    // -- POST ?action=delete&name=...
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'delete') {
        $name = $_GET['name'] ?? '';
        if ($name === '') {
            $body = readBody();
            $name = $body['name'] ?? '';
        }
        if ($name === '') jsonOut(['success' => false, 'error' => 'Missing name parameter'], 400);

        $db = getDb();
        $stmt = $db->prepare('DELETE FROM projects WHERE name = :name');
        $stmt->bindValue(':name', $name, SQLITE3_TEXT);
        $stmt->execute();
        // Keep backups intentionally; they can be cleaned manually
        $db->close();

        error_log('[Projects] Deleted "' . $name . '"');
        jsonOut(['success' => true]);
    }

    // -- GET ?action=backups&name=...
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'backups') {
        $name = $_GET['name'] ?? '';
        if ($name === '') jsonOut(['success' => false, 'error' => 'Missing name parameter'], 400);

        $db   = getDb();
        $stmt = $db->prepare('SELECT id, name, source, backed_up_at, length(html) as html_len, length(css) as css_len, length(js) as js_len FROM project_backups WHERE name = :name ORDER BY id DESC LIMIT 20');
        $stmt->bindValue(':name', $name, SQLITE3_TEXT);
        $result = $stmt->execute();
        $rows = [];
        while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
            $rows[] = $row;
        }
        $db->close();
        jsonOut(['success' => true, 'backups' => $rows]);
    }

    // -- GET ?action=restore&name=...&id=...
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'restore') {
        $name     = $_GET['name'] ?? '';
        $backupId = (int)($_GET['id'] ?? 0);
        if ($name === '' || $backupId === 0) {
            jsonOut(['success' => false, 'error' => 'Missing name or id parameter'], 400);
        }

        $db   = getDb();
        $stmt = $db->prepare('SELECT * FROM project_backups WHERE id = :id AND name = :name');
        $stmt->bindValue(':id',   $backupId, SQLITE3_INTEGER);
        $stmt->bindValue(':name', $name,     SQLITE3_TEXT);
        $backup = $stmt->execute()->fetchArray(SQLITE3_ASSOC);

        if (!$backup) {
            $db->close();
            jsonOut(['success' => false, 'error' => 'Backup not found'], 404);
        }

        // Back up the current version before restoring
        $current = $db->prepare('SELECT * FROM projects WHERE name = :name');
        $current->bindValue(':name', $name, SQLITE3_TEXT);
        $cur = $current->execute()->fetchArray(SQLITE3_ASSOC);
        if ($cur) {
            $now = date('Y-m-d H:i:s');
            $bk = $db->prepare('INSERT INTO project_backups (name, html, css, js, source, backed_up_at) VALUES (:name, :html, :css, :js, :source, :ts)');
            $bk->bindValue(':name',   $cur['name'],       SQLITE3_TEXT);
            $bk->bindValue(':html',   $cur['html'],       SQLITE3_TEXT);
            $bk->bindValue(':css',    $cur['css'],        SQLITE3_TEXT);
            $bk->bindValue(':js',     $cur['js'],         SQLITE3_TEXT);
            $bk->bindValue(':source', 'pre-restore',      SQLITE3_TEXT);
            $bk->bindValue(':ts',     $now,               SQLITE3_TEXT);
            $bk->execute();
        }

        // Restore
        $restore = $db->prepare('INSERT OR REPLACE INTO projects (name, html, css, js, source, updated_at) VALUES (:name, :html, :css, :js, :source, :ts)');
        $restore->bindValue(':name',   $backup['name'],          SQLITE3_TEXT);
        $restore->bindValue(':html',   $backup['html'],          SQLITE3_TEXT);
        $restore->bindValue(':css',    $backup['css'],           SQLITE3_TEXT);
        $restore->bindValue(':js',     $backup['js'],            SQLITE3_TEXT);
        $restore->bindValue(':source', 'restored',               SQLITE3_TEXT);
        $restore->bindValue(':ts',     date('Y-m-d H:i:s'),     SQLITE3_TEXT);
        $restore->execute();
        $db->close();

        error_log('[Projects] Restored "' . $name . '" from backup id=' . $backupId);
        jsonOut(['success' => true, 'restoredFrom' => $backupId]);
    }

    // -- GET ?action=poll_update
    // Browser asks: did Copilot save a project since I last checked?
    if ($_SERVER['REQUEST_METHOD'] === 'GET' && $action === 'poll_update') {
        $signal = readJsonFile(UPDATE_SIGNAL_FILE);
        if (!$signal) {
            jsonOut(['success' => true, 'hasUpdate' => false]);
        }
        $ack     = readJsonFile(UPDATE_ACK_FILE);
        $lastAck = $ack['updatedAt'] ?? '';
        $sigTime = $signal['updatedAt'] ?? '';
        if ($lastAck === $sigTime) {
            jsonOut(['success' => true, 'hasUpdate' => false]);
        }
        error_log('[Projects] poll_update: pending save for "' . ($signal['name'] ?? '?') . '" at ' . $sigTime);
        jsonOut([
            'success'   => true,
            'hasUpdate' => true,
            'name'      => $signal['name']      ?? '',
            'source'    => $signal['source']     ?? 'vscode-copilot',
            'updatedAt' => $sigTime,
        ]);
    }

    // -- POST ?action=ack_update
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'ack_update') {
        $signal = readJsonFile(UPDATE_SIGNAL_FILE);
        if ($signal) {
            writeJsonFile(UPDATE_ACK_FILE, [
                'updatedAt' => $signal['updatedAt'] ?? '',
                'ackedAt'   => date('Y-m-d H:i:s'),
            ]);
        }
        jsonOut(['success' => true]);
    }

    // -- POST ?action=sync_to_bridge
    // Browser sends current editor state; we save to DB + write editor files
    // back to the projects/ folder so VSCode can see the latest code.
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && $action === 'sync_to_bridge') {
        $body = json_decode(file_get_contents('php://input'), true);
        if (!$body || empty($body['name'])) {
            jsonOut(['success' => false, 'error' => 'Missing name in request body'], 400);
        }

        $name   = $body['name'];
        $html   = $body['html']   ?? '';
        $css    = $body['css']    ?? '';
        $js     = $body['js']     ?? '';
        $source = 'browser-sync';
        $now    = date('Y-m-d H:i:s');

        $db = getDb();

        // Auto-backup existing
        $existing = $db->prepare('SELECT * FROM projects WHERE name = :name');
        $existing->bindValue(':name', $name, SQLITE3_TEXT);
        $old = $existing->execute()->fetchArray(SQLITE3_ASSOC);
        if ($old) {
            $bk = $db->prepare('INSERT INTO project_backups (name, html, css, js, source, backed_up_at) VALUES (:n, :h, :c, :j, :s, :t)');
            $bk->bindValue(':n', $old['name'],   SQLITE3_TEXT);
            $bk->bindValue(':h', $old['html'],   SQLITE3_TEXT);
            $bk->bindValue(':c', $old['css'],    SQLITE3_TEXT);
            $bk->bindValue(':j', $old['js'],     SQLITE3_TEXT);
            $bk->bindValue(':s', $old['source'], SQLITE3_TEXT);
            $bk->bindValue(':t', $now,           SQLITE3_TEXT);
            $bk->execute();
        }

        // Upsert into projects table
        $ins = $db->prepare('INSERT OR REPLACE INTO projects (name, html, css, js, source, updated_at) VALUES (:n, :h, :c, :j, :s, :t)');
        $ins->bindValue(':n', $name,   SQLITE3_TEXT);
        $ins->bindValue(':h', $html,   SQLITE3_TEXT);
        $ins->bindValue(':c', $css,    SQLITE3_TEXT);
        $ins->bindValue(':j', $js,     SQLITE3_TEXT);
        $ins->bindValue(':s', $source, SQLITE3_TEXT);
        $ins->bindValue(':t', $now,    SQLITE3_TEXT);
        $ins->execute();
        $db->close();

        // Write editor files to projects/ folder.
        // DATA_DIR always points to the real vscode-bridge/data, so go up
        // one level to reach vscode-bridge/projects regardless of doc root.
        $projectsDir = realpath(DATA_DIR . '/../projects');
        if (!$projectsDir && is_dir(__DIR__ . '/../projects')) {
            $projectsDir = realpath(__DIR__ . '/../projects');
        }
        $wrote = [];
        if ($projectsDir && is_dir($projectsDir)) {
            $map = [
                'html-editor.html' => $html,
                'css-editor.css'   => $css,
                'js-editor.js'     => $js,
            ];
            foreach ($map as $fname => $content) {
                $path = $projectsDir . '/' . $fname;
                if (file_put_contents($path, $content, LOCK_EX) !== false) {
                    $wrote[] = $fname;
                } else {
                    error_log('[Projects] sync_to_bridge: failed to write ' . $path);
                }
            }
        } else {
            error_log('[Projects] sync_to_bridge: projects dir not found at ' . __DIR__ . '/../projects');
        }

        error_log('[Projects] sync_to_bridge: "' . $name . '" saved + exported (' . count($wrote) . ' files)');
        jsonOut([
            'success'   => true,
            'name'      => $name,
            'updatedAt' => $now,
            'backedUp'  => $old !== false,
            'files'     => $wrote,
        ]);
    }

    // Fallthrough
    error_log('[Projects] Unhandled: ' . $_SERVER['REQUEST_METHOD'] . ' ?action=' . $action);
    jsonOut(['success' => false, 'error' => 'Unknown action "' . $action . '"'], 404);

} catch (Throwable $e) {
    error_log('[Projects] Uncaught: ' . $e->getMessage() . ' in ' . $e->getFile() . ':' . $e->getLine());
    jsonOut(['success' => false, 'error' => 'Internal error', 'detail' => $e->getMessage()], 500);
}
