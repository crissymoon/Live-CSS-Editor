<?php
/**
 * db_bridge/connection.php
 *
 * Secure SQLite PDO connection factory.
 * Settings mirror the C layer in dev-tools/db-browser/core/db_optimization.c:
 *   - WAL journal mode
 *   - 8 MB cache
 *   - NORMAL synchronous
 *   - 256 MB mmap
 *
 * Usage:
 *   require_once __DIR__ . '/../db_bridge/connection.php';
 *   $pdo = db_connect('/path/to/file.db');
 */

declare(strict_types=1);

/**
 * Returns an open PDO handle for the given SQLite database path.
 * Throws RuntimeException on failure.
 *
 * @param string $db_path   Absolute path to the .db file.
 * @param bool   $readonly  Open read-only when true (default false).
 */
function db_connect(string $db_path, bool $readonly = false): PDO
{
    if (!file_exists($db_path)) {
        throw new RuntimeException('Database not found: ' . basename($db_path));
    }

    if (!is_readable($db_path)) {
        throw new RuntimeException('Database not readable: ' . basename($db_path));
    }

    if ($readonly) {
        $dsn = 'sqlite:file:' . rawurlencode($db_path) . '?mode=ro&uri=true';
    } else {
        $dsn = 'sqlite:' . $db_path;
    }

    $pdo = new PDO($dsn, null, null, [
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        PDO::ATTR_TIMEOUT            => 5,
    ]);

    // Match C layer optimizations from db_optimization.c
    $pdo->exec("PRAGMA journal_mode=WAL");
    $pdo->exec("PRAGMA synchronous=NORMAL");
    $pdo->exec("PRAGMA cache_size=-8000");    // 8MB (negative = kibibytes)
    $pdo->exec("PRAGMA mmap_size=268435456"); // 256MB
    $pdo->exec("PRAGMA foreign_keys=ON");
    $pdo->exec("PRAGMA temp_store=MEMORY");

    return $pdo;
}

/**
 * Returns the resolved, validated absolute path for a named database.
 * Only allows databases within the approved root to prevent path traversal.
 *
 * @param string $name  Database name e.g. "pages" or relative path "pages/main".
 * @param string $root  Approved root directory - defaults to live-css workspace.
 */
function db_resolve_path(string $name, string $root = ''): string
{
    if ($root === '') {
        $root = dirname(__DIR__); // live-css root
    }

    // Strip any .db extension if provided, we add it
    $name = preg_replace('/\.db$/i', '', $name);

    // Allow only alphanumeric, dash, underscore, and single forward slash
    if (!preg_match('/^[a-zA-Z0-9_\-\/]+$/', $name)) {
        throw new InvalidArgumentException('Invalid database name: ' . $name);
    }

    // Prevent traversal
    if (str_contains($name, '..')) {
        throw new InvalidArgumentException('Directory traversal not allowed.');
    }

    $path = realpath($root) . '/' . $name . '.db';

    // Confirm the resolved path is still under the approved root
    $resolved = realpath(dirname($path));
    if ($resolved === false || strpos($resolved, realpath($root)) !== 0) {
        throw new InvalidArgumentException('Database path outside approved root.');
    }

    return $path;
}

/**
 * Lists all .db files under the approved root (excludes build/vendor/legacy).
 *
 * @return array<array{name: string, path: string, size: int, wal: bool}>
 */
function db_list_all(string $root = ''): array
{
    if ($root === '') {
        $root = dirname(__DIR__);
    }

    $skip = ['build', 'vendor', 'node_modules', 'legacy', '.git', 'target'];
    $dbs  = [];

    $it = new RecursiveIteratorIterator(
        new RecursiveCallbackFilterIterator(
            new RecursiveDirectoryIterator($root, FilesystemIterator::SKIP_DOTS),
            function ($file, $key, $it) use ($skip) {
                if ($it->hasChildren()) {
                    $name = $file->getFilename();
                    return !in_array($name, $skip, true);
                }
                return true;
            }
        )
    );

    foreach ($it as $file) {
        if ($file->getExtension() !== 'db') {
            continue;
        }
        $abs  = $file->getRealPath();
        $rel  = ltrim(str_replace(realpath($root), '', $abs), '/');
        $wal  = file_exists($abs . '-wal');
        $dbs[] = [
            'name' => $rel,
            'path' => $abs,
            'size' => $file->getSize(),
            'wal'  => $wal,
        ];
    }

    return $dbs;
}
