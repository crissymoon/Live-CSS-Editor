#ifndef DATA_PROTECTION_H
#define DATA_PROTECTION_H

#include "../core/db_manager.h"
#include "app_state.h"
#include <stdbool.h>

typedef enum {
    QUERY_SAFE,        // SELECT, PRAGMA, EXPLAIN
    QUERY_MODERATE,    // INSERT, UPDATE with WHERE
    QUERY_DESTRUCTIVE  // DELETE, DROP, ALTER, UPDATE without WHERE
} QueryRiskLevel;

typedef struct {
    QueryRiskLevel risk_level;
    bool is_write_operation;
    bool affects_structure;   // DROP, ALTER, CREATE
    bool has_where_clause;
    char *query_type;         // "SELECT", "INSERT", "UPDATE", etc.
    char *warning_message;
} QueryAnalysis;

// Query analysis and validation
QueryAnalysis* analyze_query_risk(const char *query);
void free_query_analysis(QueryAnalysis *analysis);
bool should_confirm_query(QueryAnalysis *analysis);
char* get_confirmation_message(QueryAnalysis *analysis);

// Database backup protection
bool create_auto_backup(DBManager *mgr, const char *reason);
char* get_backup_path(const char *db_path, const char *suffix);

// Query history and auto-save
bool save_query_to_history(AppState *state, const char *query);
bool load_last_query(AppState *state, char **query_out);
bool has_unsaved_query(AppState *state);
void mark_query_saved(AppState *state);
void mark_query_dirty(AppState *state);

// Transaction helpers
bool begin_protected_transaction(DBManager *mgr);
bool commit_with_confirmation(DBManager *mgr, const char *message);
bool rollback_transaction(DBManager *mgr);

#endif // DATA_PROTECTION_H
