#ifndef SQLITE_DB_H
#define SQLITE_DB_H

#include <sqlite3.h>
#include <stdbool.h>

/* Database structure */
typedef struct LinterDB {
    sqlite3 *db;
    char *db_path;
    bool is_open;
} LinterDB;

/* Pattern structure */
typedef struct Pattern {
    int id;
    char *name;
    char *description;
    char *regex_pattern;
    char *language;
    int severity;
    bool enabled;
    struct Pattern *next;
} Pattern;

/* Tracking entry structure */
typedef struct TrackingEntry {
    int id;
    char *file_path;
    char *rule_name;
    int line_number;
    char *timestamp;
    char *issue_hash;
    struct TrackingEntry *next;
} TrackingEntry;

/* Merge change structure */
typedef struct MergeChange {
    int id;
    char *source_file;
    char *target_file;
    int line_number;
    char *old_content;
    char *new_content;
    bool approved;
    char *timestamp;
    struct MergeChange *next;
} MergeChange;

/* Database API */
LinterDB* db_create(const char *db_path);
void db_destroy(LinterDB *db);

int db_open(LinterDB *db);
int db_close(LinterDB *db);
int db_initialize_schema(LinterDB *db);

/* Pattern management */
int db_add_pattern(LinterDB *db, const char *name, const char *description,
                   const char *regex_pattern, const char *language, int severity);
Pattern* db_get_patterns(LinterDB *db, const char *language, int *count);
int db_update_pattern(LinterDB *db, int id, const char *name, const char *description,
                      const char *regex_pattern, int severity);
int db_delete_pattern(LinterDB *db, int id);
int db_enable_pattern(LinterDB *db, int id, bool enabled);
void db_free_patterns(Pattern *patterns);

/* Tracking management */
int db_track_issue(LinterDB *db, const char *file_path, const char *rule_name,
                   int line_number, const char *issue_hash);
TrackingEntry* db_get_tracked_issues(LinterDB *db, const char *file_path, int *count);
int db_clear_tracking(LinterDB *db, const char *file_path);
void db_free_tracking_entries(TrackingEntry *entries);

/* Merge management */
int db_add_merge_change(LinterDB *db, const char *source_file, const char *target_file,
                        int line_number, const char *old_content, const char *new_content);
MergeChange* db_get_pending_changes(LinterDB *db, const char *file_path, int *count);
int db_approve_change(LinterDB *db, int change_id, bool approved);
int db_get_approved_changes(LinterDB *db, const char *file_path, MergeChange **changes, int *count);
void db_free_merge_changes(MergeChange *changes);

/* Utility functions */
char* db_generate_issue_hash(const char *file_path, const char *rule_name, int line_number);
int db_execute_query(LinterDB *db, const char *query);

#endif /* SQLITE_DB_H */
