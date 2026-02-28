<?php
/**
 * Agent DB -- SQLite version store
 * Keeps the last 3 snapshots of every file the agent touches.
 * Each file_path has a rotating 3-slot buffer (slots 1, 2, 3).
 * Slot order is determined by created_at; the oldest slot is always overwritten next.
 *
 * Position tracking (for go-back / go-forward):
 *   position 0 = oldest snapshot currently held
 *   position 1 = middle
 *   position 2 = newest (current)
 *
 * Back/forward rule enforced in AgentHistory:
 *   can go back 1 step from any position, but not 2 consecutive steps back.
 */

class AgentDB
{
    private static ?PDO    $pdo     = null;
    private static ?string $dbPath  = null;

    /** Return the singleton PDO connection, creating the DB file if needed. */
    public static function pdo(): PDO
    {
        if (self::$pdo !== null) {
            return self::$pdo;
        }

        $dir    = __DIR__ . '/data';
        if (!is_dir($dir)) {
            mkdir($dir, 0755, true);
        }

        self::$dbPath = $dir . '/agent.db';
        $pdo = new PDO('sqlite:' . self::$dbPath, null, null, [
            PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]);

        $pdo->exec("PRAGMA journal_mode = WAL");
        $pdo->exec("PRAGMA foreign_keys = ON");

        self::migrate($pdo);
        self::$pdo = $pdo;
        return $pdo;
    }

    // -------------------------------------------------------------------------
    // Schema
    // -------------------------------------------------------------------------

    private static function migrate(PDO $pdo): void
    {
        $pdo->exec("
            CREATE TABLE IF NOT EXISTS file_versions (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path   TEXT    NOT NULL,
                content     TEXT    NOT NULL,
                checksum    TEXT    NOT NULL,
                label       TEXT    NOT NULL DEFAULT '',
                created_at  INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_fv_path ON file_versions(file_path, created_at);

            CREATE TABLE IF NOT EXISTS agent_history (
                id              INTEGER PRIMARY KEY AUTOINCREMENT,
                file_path       TEXT    NOT NULL,
                current_version INTEGER NOT NULL,
                last_direction  TEXT    NOT NULL DEFAULT 'forward',
                consecutive_back INTEGER NOT NULL DEFAULT 0,
                updated_at      INTEGER NOT NULL
            );
            CREATE UNIQUE INDEX IF NOT EXISTS idx_ah_path ON agent_history(file_path);
        ");
    }

    // -------------------------------------------------------------------------
    // Version storage
    // -------------------------------------------------------------------------

    /** Save a new snapshot of a file. Purges oldest if already 3 are stored. */
    public static function saveVersion(
        string $filePath,
        string $content,
        string $label = ''
    ): int {
        $pdo      = self::pdo();
        $checksum = md5($content);
        $now      = time();

        // Skip saving if content is identical to last version
        $last = self::getVersion($filePath, 0);
        if ($last && $last['checksum'] === $checksum) {
            return (int) $last['id'];
        }

        // Count existing
        $count = (int) $pdo
            ->query("SELECT COUNT(*) FROM file_versions WHERE file_path = " . $pdo->quote($filePath))
            ->fetchColumn();

        if ($count >= 3) {
            // Delete the oldest
            $oldest = $pdo->query(
                "SELECT id FROM file_versions WHERE file_path = " . $pdo->quote($filePath) .
                " ORDER BY created_at ASC LIMIT 1"
            )->fetchColumn();
            $pdo->exec("DELETE FROM file_versions WHERE id = " . (int) $oldest);
        }

        $stmt = $pdo->prepare(
            "INSERT INTO file_versions (file_path, content, checksum, label, created_at)
             VALUES (:path, :content, :checksum, :label, :now)"
        );
        $stmt->execute([
            ':path'     => $filePath,
            ':content'  => $content,
            ':checksum' => $checksum,
            ':label'    => $label,
            ':now'      => $now,
        ]);

        $newId = (int) $pdo->lastInsertId();

        // Update history pointer to newest slot
        $total = self::countVersions($filePath);
        self::setHistoryPosition($filePath, $total - 1);

        return $newId;
    }

    /**
     * Get a version by position offset from newest.
     * offset 0 = newest, 1 = second newest, 2 = oldest
     */
    public static function getVersion(string $filePath, int $offset = 0): ?array
    {
        $pdo  = self::pdo();
        $stmt = $pdo->prepare(
            "SELECT * FROM file_versions
             WHERE file_path = :path
             ORDER BY created_at DESC
             LIMIT 1 OFFSET :offset"
        );
        $stmt->execute([':path' => $filePath, ':offset' => $offset]);
        $row = $stmt->fetch();
        return $row ?: null;
    }

    /** Return all stored versions for a file (newest first, max 3). */
    public static function getAllVersions(string $filePath): array
    {
        $pdo  = self::pdo();
        $stmt = $pdo->prepare(
            "SELECT * FROM file_versions
             WHERE file_path = :path
             ORDER BY created_at DESC
             LIMIT 3"
        );
        $stmt->execute([':path' => $filePath]);
        return $stmt->fetchAll();
    }

    /** Count how many versions exist for a file. */
    public static function countVersions(string $filePath): int
    {
        $pdo = self::pdo();
        return (int) $pdo->query(
            "SELECT COUNT(*) FROM file_versions WHERE file_path = " . $pdo->quote($filePath)
        )->fetchColumn();
    }

    // -------------------------------------------------------------------------
    // History position (back/forward with rule enforcement)
    // -------------------------------------------------------------------------

    /** Get the current history row for a file, or a default. */
    public static function getHistory(string $filePath): array
    {
        $pdo  = self::pdo();
        $stmt = $pdo->prepare("SELECT * FROM agent_history WHERE file_path = :path");
        $stmt->execute([':path' => $filePath]);
        $row = $stmt->fetch();
        if (!$row) {
            return [
                'file_path'        => $filePath,
                'current_version'  => 0,
                'last_direction'   => 'forward',
                'consecutive_back' => 0,
            ];
        }
        return $row;
    }

    private static function setHistoryPosition(string $filePath, int $position): void
    {
        $pdo  = self::pdo();
        $stmt = $pdo->prepare(
            "INSERT INTO agent_history (file_path, current_version, last_direction, consecutive_back, updated_at)
             VALUES (:path, :pos, 'forward', 0, :now)
             ON CONFLICT(file_path) DO UPDATE SET
               current_version  = excluded.current_version,
               last_direction   = excluded.last_direction,
               consecutive_back = excluded.consecutive_back,
               updated_at       = excluded.updated_at"
        );
        $stmt->execute([':path' => $filePath, ':pos' => $position, ':now' => time()]);
    }

    /**
     * Navigate in version history.
     * direction: 'back' | 'forward'
     * Returns the version row at the new position, or null if blocked.
     * Rule: may not go back 2 consecutive times without a forward in between.
     * Positions: 0=oldest .. (count-1)=newest
     */
    public static function navigate(string $filePath, string $direction): ?array
    {
        $history = self::getHistory($filePath);
        $count   = self::countVersions($filePath);
        if ($count === 0) { return null; }

        $pos  = (int) $history['current_version'];
        $cBack = (int) $history['consecutive_back'];

        if ($direction === 'back') {
            // Block if already at oldest
            if ($pos <= 0) { return null; }
            // Block if already went back once without going forward
            if ($cBack >= 1) { return null; }
            $newPos     = $pos - 1;
            $newCBack   = $cBack + 1;
            $newDir     = 'back';
        } else {
            // forward
            if ($pos >= $count - 1) { return null; }
            $newPos   = $pos + 1;
            $newCBack = 0;
            $newDir   = 'forward';
        }

        $pdo  = self::pdo();
        $stmt = $pdo->prepare(
            "UPDATE agent_history SET
                current_version  = :pos,
                last_direction   = :dir,
                consecutive_back = :cback,
                updated_at       = :now
             WHERE file_path = :path"
        );
        $stmt->execute([
            ':pos'   => $newPos,
            ':dir'   => $newDir,
            ':cback' => $newCBack,
            ':now'   => time(),
            ':path'  => $filePath,
        ]);

        // offset from newest: 0=newest, (count-1)=oldest
        $offset = ($count - 1) - $newPos;
        return self::getVersion($filePath, $offset);
    }

    /** Return current active version content for a file. */
    public static function currentVersion(string $filePath): ?array
    {
        $history = self::getHistory($filePath);
        $count   = self::countVersions($filePath);
        if ($count === 0) { return null; }
        $offset = ($count - 1) - (int) $history['current_version'];
        return self::getVersion($filePath, $offset);
    }

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    public static function dbPath(): string
    {
        self::pdo();
        return self::$dbPath;
    }

    /** List all file paths that have stored versions. */
    public static function allFiles(): array
    {
        $pdo = self::pdo();
        return $pdo->query(
            "SELECT DISTINCT file_path FROM file_versions ORDER BY file_path"
        )->fetchAll(PDO::FETCH_COLUMN);
    }
}
