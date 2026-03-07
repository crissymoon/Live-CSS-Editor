<?php
// app/db/db.php -- PDO SQLite helper.
// The database lives at /data/app.db in the WASM virtual FS.
// /data is mounted as an IDBFS volume by the JS layer so it persists
// across page loads via IndexedDB.

declare(strict_types=1);

define('DB_PATH', '/data/app.db');

function db_open(): PDO
{
    static $pdo = null;
    if ($pdo !== null) {
        return $pdo;
    }

    // /data is created at IDBFS mount time by the JS layer; ensure it exists
    // in case PHP runs before the FS is fully synced.
    if (!is_dir('/data')) {
        mkdir('/data', 0755, true);
    }

    $pdo = new PDO('sqlite:' . DB_PATH, null, null, [
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
    ]);

    // Enable WAL for better concurrency and crash safety (even in WASM).
    $pdo->exec('PRAGMA journal_mode = WAL');
    $pdo->exec('PRAGMA foreign_keys = ON');

    db_migrate($pdo);

    return $pdo;
}

function db_migrate(PDO $pdo): void
{
    $pdo->exec(file_get_contents(__DIR__ . '/schema.sql'));
}
