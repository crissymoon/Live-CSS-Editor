#include "sqlite_db.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

/* Database schema */
static const char *SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS patterns ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  name TEXT NOT NULL UNIQUE,"
    "  description TEXT,"
    "  regex_pattern TEXT NOT NULL,"
    "  language TEXT,"
    "  severity INTEGER DEFAULT 1,"
    "  enabled INTEGER DEFAULT 1,"
    "  created_at TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"

    "CREATE TABLE IF NOT EXISTS tracking ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  file_path TEXT NOT NULL,"
    "  rule_name TEXT NOT NULL,"
    "  line_number INTEGER,"
    "  issue_hash TEXT NOT NULL,"
    "  timestamp TEXT DEFAULT CURRENT_TIMESTAMP,"
    "  UNIQUE(file_path, issue_hash)"
    ");"

    "CREATE TABLE IF NOT EXISTS merge_changes ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  source_file TEXT NOT NULL,"
    "  target_file TEXT NOT NULL,"
    "  line_number INTEGER,"
    "  old_content TEXT,"
    "  new_content TEXT,"
    "  approved INTEGER DEFAULT 0,"
    "  timestamp TEXT DEFAULT CURRENT_TIMESTAMP"
    ");"

    "CREATE INDEX IF NOT EXISTS idx_patterns_language ON patterns(language);"
    "CREATE INDEX IF NOT EXISTS idx_tracking_file ON tracking(file_path);"
    "CREATE INDEX IF NOT EXISTS idx_merge_target ON merge_changes(target_file);";

/* Database creation and destruction */
LinterDB* db_create(const char *db_path) {
    if (!db_path) return NULL;

    LinterDB *db = (LinterDB*)malloc(sizeof(LinterDB));
    if (!db) return NULL;

    db->db = NULL;
    db->db_path = strdup(db_path);
    db->is_open = false;

    return db;
}

void db_destroy(LinterDB *db) {
    if (!db) return;

    if (db->is_open) {
        db_close(db);
    }

    if (db->db_path) free(db->db_path);
    free(db);
}

/* Database operations */
int db_open(LinterDB *db) {
    if (!db || !db->db_path) return -1;

    int rc = sqlite3_open(db->db_path, &db->db);
    if (rc != SQLITE_OK) {
        return -1;
    }

    db->is_open = true;
    return 0;
}

int db_close(LinterDB *db) {
    if (!db || !db->is_open) return -1;

    sqlite3_close(db->db);
    db->db = NULL;
    db->is_open = false;

    return 0;
}

int db_execute_query(LinterDB *db, const char *query) {
    if (!db || !db->is_open || !query) return -1;

    char *err_msg = NULL;
    int rc = sqlite3_exec(db->db, query, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        if (err_msg) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
            sqlite3_free(err_msg);
        }
        return -1;
    }

    return 0;
}

int db_initialize_schema(LinterDB *db) {
    if (!db || !db->is_open) return -1;

    return db_execute_query(db, SCHEMA_SQL);
}

/* Pattern management */
int db_add_pattern(LinterDB *db, const char *name, const char *description,
                   const char *regex_pattern, const char *language, int severity) {
    if (!db || !db->is_open || !name || !regex_pattern) return -1;

    const char *sql = "INSERT INTO patterns (name, description, regex_pattern, language, severity) "
                     "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description ? description : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, regex_pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, language ? language : "any", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, severity);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

Pattern* db_get_patterns(LinterDB *db, const char *language, int *count) {
    if (!db || !db->is_open || !count) return NULL;

    const char *sql;
    sqlite3_stmt *stmt;

    if (language) {
        sql = "SELECT id, name, description, regex_pattern, language, severity, enabled "
              "FROM patterns WHERE (language = ? OR language = 'any') AND enabled = 1;";
        sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
        sqlite3_bind_text(stmt, 1, language, -1, SQLITE_STATIC);
    } else {
        sql = "SELECT id, name, description, regex_pattern, language, severity, enabled "
              "FROM patterns WHERE enabled = 1;";
        sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);
    }

    Pattern *head = NULL;
    Pattern *tail = NULL;
    int pattern_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Pattern *pattern = (Pattern*)malloc(sizeof(Pattern));
        if (!pattern) continue;

        pattern->id = sqlite3_column_int(stmt, 0);
        pattern->name = strdup((const char*)sqlite3_column_text(stmt, 1));
        pattern->description = strdup((const char*)sqlite3_column_text(stmt, 2));
        pattern->regex_pattern = strdup((const char*)sqlite3_column_text(stmt, 3));
        pattern->language = strdup((const char*)sqlite3_column_text(stmt, 4));
        pattern->severity = sqlite3_column_int(stmt, 5);
        pattern->enabled = sqlite3_column_int(stmt, 6);
        pattern->next = NULL;

        if (!head) {
            head = pattern;
            tail = pattern;
        } else {
            tail->next = pattern;
            tail = pattern;
        }

        pattern_count++;
    }

    sqlite3_finalize(stmt);
    *count = pattern_count;

    return head;
}

int db_update_pattern(LinterDB *db, int id, const char *name, const char *description,
                      const char *regex_pattern, int severity) {
    if (!db || !db->is_open) return -1;

    const char *sql = "UPDATE patterns SET name = ?, description = ?, "
                     "regex_pattern = ?, severity = ? WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, regex_pattern, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, severity);
    sqlite3_bind_int(stmt, 5, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_delete_pattern(LinterDB *db, int id) {
    if (!db || !db->is_open) return -1;

    const char *sql = "DELETE FROM patterns WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_enable_pattern(LinterDB *db, int id, bool enabled) {
    if (!db || !db->is_open) return -1;

    const char *sql = "UPDATE patterns SET enabled = ? WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, enabled ? 1 : 0);
    sqlite3_bind_int(stmt, 2, id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

void db_free_patterns(Pattern *patterns) {
    Pattern *current = patterns;
    while (current) {
        Pattern *next = current->next;
        if (current->name) free(current->name);
        if (current->description) free(current->description);
        if (current->regex_pattern) free(current->regex_pattern);
        if (current->language) free(current->language);
        free(current);
        current = next;
    }
}

/* Hash generation */
char* db_generate_issue_hash(const char *file_path, const char *rule_name, int line_number) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "%s:%s:%d", file_path, rule_name, line_number);

    /* Simple hash - in production, use proper hash function */
    unsigned long hash = 5381;
    char *str = buffer;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }

    char *hash_str = (char*)malloc(32);
    snprintf(hash_str, 32, "%016lx", hash);

    return hash_str;
}

/* Tracking management */
int db_track_issue(LinterDB *db, const char *file_path, const char *rule_name,
                   int line_number, const char *issue_hash) {
    if (!db || !db->is_open || !file_path || !rule_name) return -1;

    char *hash = issue_hash ? strdup(issue_hash) :
                 db_generate_issue_hash(file_path, rule_name, line_number);

    const char *sql = "INSERT OR REPLACE INTO tracking (file_path, rule_name, line_number, issue_hash) "
                     "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        if (!issue_hash) free(hash);
        return -1;
    }

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, rule_name, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, line_number);
    sqlite3_bind_text(stmt, 4, hash, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (!issue_hash) free(hash);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

TrackingEntry* db_get_tracked_issues(LinterDB *db, const char *file_path, int *count) {
    if (!db || !db->is_open || !count) return NULL;

    const char *sql = "SELECT id, file_path, rule_name, line_number, timestamp, issue_hash "
                     "FROM tracking WHERE file_path = ? ORDER BY line_number;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);

    TrackingEntry *head = NULL;
    TrackingEntry *tail = NULL;
    int entry_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        TrackingEntry *entry = (TrackingEntry*)malloc(sizeof(TrackingEntry));
        if (!entry) continue;

        entry->id = sqlite3_column_int(stmt, 0);
        entry->file_path = strdup((const char*)sqlite3_column_text(stmt, 1));
        entry->rule_name = strdup((const char*)sqlite3_column_text(stmt, 2));
        entry->line_number = sqlite3_column_int(stmt, 3);
        entry->timestamp = strdup((const char*)sqlite3_column_text(stmt, 4));
        entry->issue_hash = strdup((const char*)sqlite3_column_text(stmt, 5));
        entry->next = NULL;

        if (!head) {
            head = entry;
            tail = entry;
        } else {
            tail->next = entry;
            tail = entry;
        }

        entry_count++;
    }

    sqlite3_finalize(stmt);
    *count = entry_count;

    return head;
}

int db_clear_tracking(LinterDB *db, const char *file_path) {
    if (!db || !db->is_open) return -1;

    const char *sql = file_path ?
                     "DELETE FROM tracking WHERE file_path = ?;" :
                     "DELETE FROM tracking;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    if (file_path) {
        sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);
    }

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

void db_free_tracking_entries(TrackingEntry *entries) {
    TrackingEntry *current = entries;
    while (current) {
        TrackingEntry *next = current->next;
        if (current->file_path) free(current->file_path);
        if (current->rule_name) free(current->rule_name);
        if (current->timestamp) free(current->timestamp);
        if (current->issue_hash) free(current->issue_hash);
        free(current);
        current = next;
    }
}

/* Merge management */
int db_add_merge_change(LinterDB *db, const char *source_file, const char *target_file,
                        int line_number, const char *old_content, const char *new_content) {
    if (!db || !db->is_open || !source_file || !target_file) return -1;

    const char *sql = "INSERT INTO merge_changes (source_file, target_file, line_number, "
                     "old_content, new_content) VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, source_file, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, target_file, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, line_number);
    sqlite3_bind_text(stmt, 4, old_content ? old_content : "", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, new_content ? new_content : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? sqlite3_last_insert_rowid(db->db) : -1;
}

MergeChange* db_get_pending_changes(LinterDB *db, const char *file_path, int *count) {
    if (!db || !db->is_open || !count) return NULL;

    const char *sql = "SELECT id, source_file, target_file, line_number, old_content, "
                     "new_content, approved, timestamp FROM merge_changes "
                     "WHERE target_file = ? AND approved = 0 ORDER BY line_number;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return NULL;

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);

    MergeChange *head = NULL;
    MergeChange *tail = NULL;
    int change_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MergeChange *change = (MergeChange*)malloc(sizeof(MergeChange));
        if (!change) continue;

        change->id = sqlite3_column_int(stmt, 0);
        change->source_file = strdup((const char*)sqlite3_column_text(stmt, 1));
        change->target_file = strdup((const char*)sqlite3_column_text(stmt, 2));
        change->line_number = sqlite3_column_int(stmt, 3);
        change->old_content = strdup((const char*)sqlite3_column_text(stmt, 4));
        change->new_content = strdup((const char*)sqlite3_column_text(stmt, 5));
        change->approved = sqlite3_column_int(stmt, 6);
        change->timestamp = strdup((const char*)sqlite3_column_text(stmt, 7));
        change->next = NULL;

        if (!head) {
            head = change;
            tail = change;
        } else {
            tail->next = change;
            tail = change;
        }

        change_count++;
    }

    sqlite3_finalize(stmt);
    *count = change_count;

    return head;
}

int db_approve_change(LinterDB *db, int change_id, bool approved) {
    if (!db || !db->is_open) return -1;

    const char *sql = "UPDATE merge_changes SET approved = ? WHERE id = ?;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, approved ? 1 : -1);
    sqlite3_bind_int(stmt, 2, change_id);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_get_approved_changes(LinterDB *db, const char *file_path, MergeChange **changes, int *count) {
    if (!db || !db->is_open || !changes || !count) return -1;

    const char *sql = "SELECT id, source_file, target_file, line_number, old_content, "
                     "new_content, approved, timestamp FROM merge_changes "
                     "WHERE target_file = ? AND approved = 1 ORDER BY line_number;";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, file_path, -1, SQLITE_STATIC);

    MergeChange *head = NULL;
    MergeChange *tail = NULL;
    int change_count = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MergeChange *change = (MergeChange*)malloc(sizeof(MergeChange));
        if (!change) continue;

        change->id = sqlite3_column_int(stmt, 0);
        change->source_file = strdup((const char*)sqlite3_column_text(stmt, 1));
        change->target_file = strdup((const char*)sqlite3_column_text(stmt, 2));
        change->line_number = sqlite3_column_int(stmt, 3);
        change->old_content = strdup((const char*)sqlite3_column_text(stmt, 4));
        change->new_content = strdup((const char*)sqlite3_column_text(stmt, 5));
        change->approved = sqlite3_column_int(stmt, 6);
        change->timestamp = strdup((const char*)sqlite3_column_text(stmt, 7));
        change->next = NULL;

        if (!head) {
            head = change;
            tail = change;
        } else {
            tail->next = change;
            tail = change;
        }

        change_count++;
    }

    sqlite3_finalize(stmt);
    *changes = head;
    *count = change_count;

    return 0;
}

void db_free_merge_changes(MergeChange *changes) {
    MergeChange *current = changes;
    while (current) {
        MergeChange *next = current->next;
        if (current->source_file) free(current->source_file);
        if (current->target_file) free(current->target_file);
        if (current->old_content) free(current->old_content);
        if (current->new_content) free(current->new_content);
        if (current->timestamp) free(current->timestamp);
        free(current);
        current = next;
    }
}
