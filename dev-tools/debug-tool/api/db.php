<?php
/**
 * debug-tool/api/db.php
 * SQLite3 database manager for the debug/error tracking tool.
 * All methods include console-style error handling via error_log().
 */

define('DEBUG_DB_PATH', __DIR__ . '/../db/errors.db');
define('DEBUG_DB_DIR',  __DIR__ . '/../db');

class DebugDB {

    private SQLite3 $db;

    // Valid levels and statuses - used for validation throughout
    const LEVELS   = ['critical', 'high', 'medium', 'low', 'info'];
    const STATUSES = ['open', 'pending', 'in_progress', 'fixed', 'closed', 'wontfix'];

    public function __construct() {
        try {
            if (!is_dir(DEBUG_DB_DIR)) {
                if (!mkdir(DEBUG_DB_DIR, 0755, true)) {
                    error_log('[DebugDB] FATAL: Could not create DB directory: ' . DEBUG_DB_DIR);
                    throw new RuntimeException('Cannot create DB directory.');
                }
            }

            $this->db = new SQLite3(DEBUG_DB_PATH);
            $this->db->enableExceptions(true);
            $this->db->exec('PRAGMA journal_mode = WAL;');
            $this->db->exec('PRAGMA foreign_keys = ON;');
            $this->setupSchema();

        } catch (Throwable $e) {
            error_log('[DebugDB] Constructor failed: ' . $e->getMessage());
            throw $e;
        }
    }

    // -------------------------------------------------------------------------
    // Schema
    // -------------------------------------------------------------------------
    private function setupSchema(): void {
        try {
            $this->db->exec("
                CREATE TABLE IF NOT EXISTS errors (
                    id           INTEGER PRIMARY KEY AUTOINCREMENT,
                    ticket_id    TEXT    NOT NULL UNIQUE,
                    level        TEXT    NOT NULL DEFAULT 'info',
                    status       TEXT    NOT NULL DEFAULT 'open',
                    title        TEXT    NOT NULL,
                    message      TEXT    NOT NULL,
                    source       TEXT    DEFAULT NULL,
                    file         TEXT    DEFAULT NULL,
                    line         INTEGER DEFAULT NULL,
                    stack_trace  TEXT    DEFAULT NULL,
                    context      TEXT    DEFAULT NULL,
                    ai_analysis  TEXT    DEFAULT NULL,
                    created_at   TEXT    NOT NULL DEFAULT (datetime('now')),
                    updated_at   TEXT    NOT NULL DEFAULT (datetime('now')),
                    resolved_at  TEXT    DEFAULT NULL
                );

                CREATE INDEX IF NOT EXISTS idx_level    ON errors (level);
                CREATE INDEX IF NOT EXISTS idx_status   ON errors (status);
                CREATE INDEX IF NOT EXISTS idx_source   ON errors (source);
                CREATE INDEX IF NOT EXISTS idx_created  ON errors (created_at);
            ");
        } catch (Throwable $e) {
            error_log('[DebugDB] setupSchema failed: ' . $e->getMessage());
            throw $e;
        }
    }

    // -------------------------------------------------------------------------
    // Ticket ID generator  ERR-YYYYMMDD-XXXXXX
    // -------------------------------------------------------------------------
    private function generateTicketId(): string {
        return 'ERR-' . date('Ymd') . '-' . strtoupper(substr(bin2hex(random_bytes(3)), 0, 6));
    }

    // -------------------------------------------------------------------------
    // Insert a new error record
    // -------------------------------------------------------------------------
    public function insertError(array $data): array {
        try {
            $level  = in_array($data['level']  ?? '', self::LEVELS,   true) ? $data['level']   : 'info';
            $status = in_array($data['status'] ?? '', self::STATUSES, true) ? $data['status']  : 'open';

            if (empty(trim($data['title'] ?? ''))) {
                error_log('[DebugDB] insertError: title is required');
                return ['success' => false, 'error' => 'title is required'];
            }
            if (empty(trim($data['message'] ?? ''))) {
                error_log('[DebugDB] insertError: message is required');
                return ['success' => false, 'error' => 'message is required'];
            }

            $ticketId = $this->generateTicketId();

            $context = isset($data['context']) && is_array($data['context'])
                ? json_encode($data['context'], JSON_UNESCAPED_UNICODE)
                : ($data['context'] ?? null);

            $stmt = $this->db->prepare("
                INSERT INTO errors (ticket_id, level, status, title, message, source, file, line, stack_trace, context)
                VALUES (:ticket_id, :level, :status, :title, :message, :source, :file, :line, :stack_trace, :context)
            ");

            $stmt->bindValue(':ticket_id',   $ticketId);
            $stmt->bindValue(':level',        $level);
            $stmt->bindValue(':status',       $status);
            $stmt->bindValue(':title',        trim($data['title']));
            $stmt->bindValue(':message',      trim($data['message']));
            $stmt->bindValue(':source',       $data['source']      ?? null, SQLITE3_TEXT);
            $stmt->bindValue(':file',         $data['file']        ?? null, SQLITE3_TEXT);
            $stmt->bindValue(':line',         $data['line']        ?? null, SQLITE3_INTEGER);
            $stmt->bindValue(':stack_trace',  $data['stack_trace'] ?? null, SQLITE3_TEXT);
            $stmt->bindValue(':context',      $context,                     SQLITE3_TEXT);

            $stmt->execute();
            $id = $this->db->lastInsertRowID();

            error_log("[DebugDB] New ticket created: $ticketId (level=$level, status=$status)");
            return ['success' => true, 'id' => $id, 'ticket_id' => $ticketId];

        } catch (Throwable $e) {
            error_log('[DebugDB] insertError exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage()];
        }
    }

    // -------------------------------------------------------------------------
    // List errors with optional filters and pagination
    // -------------------------------------------------------------------------
    public function listErrors(array $filters = []): array {
        try {
            $where  = [];
            $params = [];

            if (!empty($filters['level']) && in_array($filters['level'], self::LEVELS, true)) {
                $where[]           = 'level = :level';
                $params[':level']  = $filters['level'];
            }
            if (!empty($filters['status']) && in_array($filters['status'], self::STATUSES, true)) {
                $where[]            = 'status = :status';
                $params[':status']  = $filters['status'];
            }
            if (!empty($filters['source'])) {
                $where[]            = 'source = :source';
                $params[':source']  = $filters['source'];
            }
            if (!empty($filters['search'])) {
                $where[]            = '(title LIKE :search OR message LIKE :search)';
                $params[':search']  = '%' . $filters['search'] . '%';
            }

            $whereClause = count($where) ? 'WHERE ' . implode(' AND ', $where) : '';
            $limit       = max(1, min(200, (int) ($filters['limit']  ?? 50)));
            $offset      = max(0,          (int) ($filters['offset'] ?? 0));
            $order       = ($filters['order'] ?? 'desc') === 'asc' ? 'ASC' : 'DESC';

            $sql  = "SELECT * FROM errors $whereClause ORDER BY created_at $order LIMIT :limit OFFSET :offset";
            $stmt = $this->db->prepare($sql);

            foreach ($params as $key => $val) {
                $stmt->bindValue($key, $val);
            }
            $stmt->bindValue(':limit',  $limit,  SQLITE3_INTEGER);
            $stmt->bindValue(':offset', $offset, SQLITE3_INTEGER);

            $result = $stmt->execute();
            $rows   = [];
            while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
                $row['context'] = $row['context'] ? json_decode($row['context'], true) : null;
                $rows[] = $row;
            }

            // Total count for pagination
            $countSql  = "SELECT COUNT(*) as total FROM errors $whereClause";
            $countStmt = $this->db->prepare($countSql);
            foreach ($params as $key => $val) {
                $countStmt->bindValue($key, $val);
            }
            $countResult = $countStmt->execute()->fetchArray(SQLITE3_ASSOC);

            return [
                'success' => true,
                'total'   => (int) $countResult['total'],
                'limit'   => $limit,
                'offset'  => $offset,
                'data'    => $rows
            ];

        } catch (Throwable $e) {
            error_log('[DebugDB] listErrors exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage(), 'data' => []];
        }
    }

    // -------------------------------------------------------------------------
    // Get single error by id or ticket_id
    // -------------------------------------------------------------------------
    public function getError(string $identifier): array {
        try {
            $col  = is_numeric($identifier) ? 'id' : 'ticket_id';
            $stmt = $this->db->prepare("SELECT * FROM errors WHERE $col = :id LIMIT 1");
            $stmt->bindValue(':id', $identifier);
            $row  = $stmt->execute()->fetchArray(SQLITE3_ASSOC);

            if (!$row) {
                return ['success' => false, 'error' => 'Not found'];
            }
            $row['context'] = $row['context'] ? json_decode($row['context'], true) : null;
            return ['success' => true, 'data' => $row];

        } catch (Throwable $e) {
            error_log('[DebugDB] getError exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage()];
        }
    }

    // -------------------------------------------------------------------------
    // Update status / ai_analysis on a ticket
    // -------------------------------------------------------------------------
    public function updateError(string $identifier, array $data): array {
        try {
            $allowed  = ['status', 'ai_analysis', 'level', 'title', 'message'];
            $setParts = [];
            $params   = [];

            foreach ($allowed as $field) {
                if (!isset($data[$field])) continue;
                if ($field === 'status' && !in_array($data[$field], self::STATUSES, true)) {
                    error_log("[DebugDB] updateError: invalid status '{$data[$field]}'");
                    return ['success' => false, 'error' => 'Invalid status value'];
                }
                if ($field === 'level' && !in_array($data[$field], self::LEVELS, true)) {
                    error_log("[DebugDB] updateError: invalid level '{$data[$field]}'");
                    return ['success' => false, 'error' => 'Invalid level value'];
                }
                $setParts[]         = "$field = :$field";
                $params[":$field"]  = $data[$field];
            }

            if (empty($setParts)) {
                return ['success' => false, 'error' => 'No valid fields to update'];
            }

            // Set resolved_at automatically when status moves to fixed/closed
            $newStatus = $data['status'] ?? null;
            if ($newStatus === 'fixed' || $newStatus === 'closed') {
                $setParts[]              = 'resolved_at = :resolved_at';
                $params[':resolved_at']  = date('Y-m-d H:i:s');
            }

            $setParts[] = "updated_at = datetime('now')";

            $col    = is_numeric($identifier) ? 'id' : 'ticket_id';
            $sql    = 'UPDATE errors SET ' . implode(', ', $setParts) . " WHERE $col = :id";
            $stmt   = $this->db->prepare($sql);
            $stmt->bindValue(':id', $identifier);
            foreach ($params as $k => $v) {
                $stmt->bindValue($k, $v);
            }
            $stmt->execute();

            $changed = $this->db->changes();
            if ($changed === 0) {
                error_log("[DebugDB] updateError: no rows matched for '$identifier'");
                return ['success' => false, 'error' => 'No rows updated - ticket not found'];
            }

            error_log("[DebugDB] Ticket $identifier updated (" . implode(', ', array_keys($data)) . ")");
            return ['success' => true, 'changed' => $changed];

        } catch (Throwable $e) {
            error_log('[DebugDB] updateError exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage()];
        }
    }

    // -------------------------------------------------------------------------
    // Delete a ticket (use with caution)
    // -------------------------------------------------------------------------
    public function deleteError(string $identifier): array {
        try {
            $col  = is_numeric($identifier) ? 'id' : 'ticket_id';
            $stmt = $this->db->prepare("DELETE FROM errors WHERE $col = :id");
            $stmt->bindValue(':id', $identifier);
            $stmt->execute();
            $changed = $this->db->changes();
            if ($changed === 0) {
                return ['success' => false, 'error' => 'Ticket not found'];
            }
            error_log("[DebugDB] Ticket $identifier deleted.");
            return ['success' => true, 'deleted' => $changed];
        } catch (Throwable $e) {
            error_log('[DebugDB] deleteError exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage()];
        }
    }

    // -------------------------------------------------------------------------
    // Summary stats
    // -------------------------------------------------------------------------
    public function stats(): array {
        try {
            $rows = [];
            foreach (self::LEVELS as $level) {
                $rows['by_level'][$level] = 0;
            }
            foreach (self::STATUSES as $status) {
                $rows['by_status'][$status] = 0;
            }

            $res = $this->db->query("SELECT level, COUNT(*) as n FROM errors GROUP BY level");
            while ($row = $res->fetchArray(SQLITE3_ASSOC)) {
                $rows['by_level'][$row['level']] = (int) $row['n'];
            }

            $res = $this->db->query("SELECT status, COUNT(*) as n FROM errors GROUP BY status");
            while ($row = $res->fetchArray(SQLITE3_ASSOC)) {
                $rows['by_status'][$row['status']] = (int) $row['n'];
            }

            $total = $this->db->querySingle("SELECT COUNT(*) FROM errors");
            $rows['total'] = (int) $total;

            return ['success' => true, 'stats' => $rows];
        } catch (Throwable $e) {
            error_log('[DebugDB] stats exception: ' . $e->getMessage());
            return ['success' => false, 'error' => $e->getMessage()];
        }
    }

    public function getDB(): SQLite3 {
        return $this->db;
    }
}
