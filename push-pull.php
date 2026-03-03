#!/usr/bin/env php
<?php
/**
 * push-pull.php -- Push and pull projects between vscode-bridge/projects/ and
 * the SQLite database that the Live CSS Tool reads from.
 *
 * Usage:
 *   php push-pull.php push [name]   Read files from vscode-bridge/projects/ and
 *                                   save them into the SQLite database (same way
 *                                   the Save button in the tool does).
 *
 *   php push-pull.php pull [name]   Read a project from the SQLite database and
 *                                   write it to vscode-bridge/projects/ as
 *                                   html-editor.html, css-editor.css, js-editor.js.
 *
 *   php push-pull.php list          List all saved projects in the database.
 *
 * Default project name: crissys-style-tool
 *
 * After a push, you can open the tool, click Load, and select the project.
 * After a pull, you can edit the files in VSCode and push them back.
 *
 * Crissy's Style Tool
 * Copyright (c) 2026 Crissy Deutsch / XcaliburMoon Web Development
 * MIT License -- see LICENSE file for full text.
 */

declare(strict_types=1);

// ---------------------------------------------------------------------------
// Paths
// ---------------------------------------------------------------------------

define('PROJECT_ROOT', __DIR__);
define('BRIDGE_DIR',   PROJECT_ROOT . '/vscode-bridge');
define('PROJECTS_DIR', BRIDGE_DIR   . '/projects');
define('DATA_DIR',     BRIDGE_DIR   . '/data');
define('DB_PATH',      DATA_DIR     . '/projects.db');

define('HTML_FILE', PROJECTS_DIR . '/html-editor.html');
define('CSS_FILE',  PROJECTS_DIR . '/css-editor.css');
define('JS_FILE',   PROJECTS_DIR . '/js-editor.js');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function info(string $msg): void  { echo "[push-pull] $msg\n"; }
function fail(string $msg): void  { fwrite(STDERR, "[push-pull] ERROR: $msg\n"); exit(1); }

function ensureDataDir(): void {
    if (!is_dir(DATA_DIR)) {
        if (!mkdir(DATA_DIR, 0755, true)) {
            fail('Could not create data directory: ' . DATA_DIR);
        }
        info('Created data directory: ' . DATA_DIR);
    }
}

/**
 * Open (or create) the SQLite database with the same schema
 * the browser API uses.
 */
function getDb(): SQLite3 {
    ensureDataDir();

    $db = new SQLite3(DB_PATH);
    $db->busyTimeout(3000);
    $db->exec('PRAGMA journal_mode=WAL');
    $db->exec('PRAGMA foreign_keys=ON');

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

    $db->exec('
        CREATE TABLE IF NOT EXISTS project_backups (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            name         TEXT NOT NULL,
            html         TEXT NOT NULL DEFAULT "",
            css          TEXT NOT NULL DEFAULT "",
            js           TEXT NOT NULL DEFAULT "",
            source       TEXT NOT NULL DEFAULT "browser",
            backed_up_at TEXT NOT NULL
        )
    ');
    $db->exec('CREATE INDEX IF NOT EXISTS idx_backups_name ON project_backups(name)');

    return $db;
}

/**
 * Write the update signal file so the browser can detect the change
 * when it polls via ?action=poll_update.
 */
function writeUpdateSignal(string $name, string $source): void {
    $signalFile = DATA_DIR . '/project-update-signal.json';
    $data = [
        'name'      => $name,
        'source'    => $source,
        'updatedAt' => date('Y-m-d H:i:s'),
    ];
    file_put_contents($signalFile, json_encode($data, JSON_PRETTY_PRINT), LOCK_EX);
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

/**
 * PUSH: Read files from vscode-bridge/projects/ and save to SQLite.
 * This mirrors what happens when the user clicks Save in the tool.
 */
function doPush(string $name): void {
    // Verify source files exist
    if (!file_exists(HTML_FILE)) fail('Missing file: ' . HTML_FILE);
    if (!file_exists(CSS_FILE))  fail('Missing file: ' . CSS_FILE);

    $html = file_get_contents(HTML_FILE);
    $css  = file_get_contents(CSS_FILE);
    $js   = file_exists(JS_FILE) ? file_get_contents(JS_FILE) : '';
    $now  = date('Y-m-d H:i:s');

    if ($html === false) fail('Could not read ' . HTML_FILE);
    if ($css  === false) fail('Could not read ' . CSS_FILE);

    info('Reading project files from vscode-bridge/projects/');
    info('  html-editor.html  ' . strlen($html) . ' bytes');
    info('  css-editor.css    ' . strlen($css)  . ' bytes');
    info('  js-editor.js      ' . strlen($js)   . ' bytes');

    $db = getDb();

    // Auto-backup: if the project already exists, copy it to backups first
    $existing = $db->prepare('SELECT * FROM projects WHERE name = :name');
    $existing->bindValue(':name', $name, SQLITE3_TEXT);
    $old = $existing->execute()->fetchArray(SQLITE3_ASSOC);

    $backedUp = false;
    if ($old) {
        $bk = $db->prepare(
            'INSERT INTO project_backups (name, html, css, js, source, backed_up_at) '
            . 'VALUES (:name, :html, :css, :js, :source, :ts)'
        );
        $bk->bindValue(':name',   $old['name'],   SQLITE3_TEXT);
        $bk->bindValue(':html',   $old['html'],   SQLITE3_TEXT);
        $bk->bindValue(':css',    $old['css'],     SQLITE3_TEXT);
        $bk->bindValue(':js',     $old['js'],      SQLITE3_TEXT);
        $bk->bindValue(':source', $old['source'],  SQLITE3_TEXT);
        $bk->bindValue(':ts',     $now,            SQLITE3_TEXT);
        $bk->execute();
        $backedUp = true;
        info('Backed up previous version of "' . $name . '"');
    }

    // Upsert the project
    $ins = $db->prepare(
        'INSERT OR REPLACE INTO projects (name, html, css, js, source, updated_at) '
        . 'VALUES (:name, :html, :css, :js, :source, :ts)'
    );
    $ins->bindValue(':name',   $name,             SQLITE3_TEXT);
    $ins->bindValue(':html',   $html,             SQLITE3_TEXT);
    $ins->bindValue(':css',    $css,              SQLITE3_TEXT);
    $ins->bindValue(':js',     $js,              SQLITE3_TEXT);
    $ins->bindValue(':source', 'vscode-bridge',   SQLITE3_TEXT);
    $ins->bindValue(':ts',     $now,              SQLITE3_TEXT);
    $ins->execute();

    $db->close();

    // Signal the browser that a project was updated
    writeUpdateSignal($name, 'vscode-bridge');

    info('Saved "' . $name . '" to database at ' . $now . ($backedUp ? ' (backup created)' : ''));
    info('Open the tool, click Load, and select "' . $name . '" to see it.');
}

/**
 * PULL: Read a project from SQLite and write to vscode-bridge/projects/.
 * Writes html-editor.html, css-editor.css, js-editor.js.
 */
function doPull(string $name): void {
    if (!file_exists(DB_PATH)) {
        fail('No database found at ' . DB_PATH . '. Save a project in the tool first, or run push first.');
    }

    $db   = getDb();
    $stmt = $db->prepare('SELECT * FROM projects WHERE name = :name');
    $stmt->bindValue(':name', $name, SQLITE3_TEXT);
    $row = $stmt->execute()->fetchArray(SQLITE3_ASSOC);
    $db->close();

    if (!$row) {
        fail('Project "' . $name . '" not found in database. Use "php push-pull.php list" to see available projects.');
    }

    $html = $row['html'] ?? '';
    $css  = $row['css']  ?? '';
    $js   = $row['js']   ?? '';

    info('Pulling "' . $name . '" from database');
    info('  source:     ' . ($row['source'] ?? 'unknown'));
    info('  updated_at: ' . ($row['updated_at'] ?? 'unknown'));

    // Make sure the projects directory exists
    if (!is_dir(PROJECTS_DIR)) {
        mkdir(PROJECTS_DIR, 0755, true);
    }

    file_put_contents(HTML_FILE, $html);
    file_put_contents(CSS_FILE,  $css);
    file_put_contents(JS_FILE,   $js);

    info('Wrote files to vscode-bridge/projects/:');
    info('  html-editor.html  ' . strlen($html) . ' bytes');
    info('  css-editor.css    ' . strlen($css)  . ' bytes');
    info('  js-editor.js      ' . strlen($js)   . ' bytes');
    info('Files are ready to edit. Run "php push-pull.php push" when done.');
}

/**
 * LIST: Show all projects in the database.
 */
function doList(): void {
    if (!file_exists(DB_PATH)) {
        fail('No database found at ' . DB_PATH . '. Save a project in the tool first, or run push first.');
    }

    $db   = getDb();
    $stmt = $db->query(
        'SELECT name, source, updated_at, length(html) as html_len, length(css) as css_len, length(js) as js_len '
        . 'FROM projects ORDER BY name'
    );

    $rows = [];
    while ($row = $stmt->fetchArray(SQLITE3_ASSOC)) {
        $rows[] = $row;
    }
    $db->close();

    if (empty($rows)) {
        info('No projects found in database.');
        return;
    }

    info('Projects in database (' . count($rows) . '):');
    echo str_pad('Name', 30) . str_pad('Source', 18) . str_pad('Updated', 22) . str_pad('HTML', 8) . str_pad('CSS', 8) . 'JS' . "\n";
    echo str_repeat('-', 94) . "\n";

    foreach ($rows as $r) {
        echo str_pad($r['name'], 30)
           . str_pad($r['source'], 18)
           . str_pad($r['updated_at'], 22)
           . str_pad($r['html_len'] . 'b', 8)
           . str_pad($r['css_len'] . 'b', 8)
           . ($r['js_len'] ?? 0) . 'b'
           . "\n";
    }
}

// ---------------------------------------------------------------------------
// Usage
// ---------------------------------------------------------------------------

function usage(): void {
    echo <<<USAGE
push-pull.php -- Push and pull projects for Crissy's Style Tool

Usage:
  php push-pull.php push [name]   Save vscode-bridge/projects/ files to the database
  php push-pull.php pull [name]   Write a database project to vscode-bridge/projects/
  php push-pull.php list          List all projects in the database

Default project name: crissys-style-tool

After push:  Open the tool -> Load -> select the project name
After pull:  Edit files in vscode-bridge/projects/ -> push when done

USAGE;
    exit(0);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

$cmd  = $argv[1] ?? '';
$name = $argv[2] ?? 'crissys-style-tool';

if ($cmd === '' || $cmd === '-h' || $cmd === '--help') {
    usage();
}

switch ($cmd) {
    case 'push':
        doPush($name);
        break;
    case 'pull':
        doPull($name);
        break;
    case 'list':
        doList();
        break;
    default:
        fwrite(STDERR, "[push-pull] Unknown command: $cmd\n\n");
        usage();
}
