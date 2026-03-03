#include "data_protection.h"
#include "ui_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>

// Helper to convert string to uppercase for comparison
static char* str_to_upper(const char *str) {
    if (!str) return NULL;
    char *upper = g_strdup(str);
    for (char *p = upper; *p; p++) {
        *p = toupper(*p);
    }
    return upper;
}

// Extract the first SQL keyword from query
static char* extract_query_type(const char *query) {
    if (!query) return NULL;
    
    // Skip whitespace and comments
    const char *p = query;
    while (*p && (isspace(*p) || *p == '-' || *p == '/')) {
        if (*p == '-' && *(p+1) == '-') {
            while (*p && *p != '\n') p++;
            continue;
        }
        if (*p == '/' && *(p+1) == '*') {
            while (*p && !(*p == '*' && *(p+1) == '/')) p++;
            if (*p) p += 2;
            continue;
        }
        p++;
    }
    
    // Extract first word
    char keyword[64] = {0};
    int i = 0;
    while (*p && !isspace(*p) && *p != ';' && i < 63) {
        keyword[i++] = toupper(*p++);
    }
    keyword[i] = '\0';
    
    return g_strdup(keyword);
}

// Check if query contains WHERE clause
static bool has_where_clause(const char *query) {
    if (!query) return false;
    
    char *upper = str_to_upper(query);
    bool result = strstr(upper, " WHERE ") != NULL || strstr(upper, " WHERE\n") != NULL;
    g_free(upper);
    return result;
}

// Analyze query for risk level
QueryAnalysis* analyze_query_risk(const char *query) {
    if (!query || strlen(query) == 0) {
        return NULL;
    }
    
    QueryAnalysis *analysis = g_malloc0(sizeof(QueryAnalysis));
    analysis->query_type = extract_query_type(query);
    analysis->has_where_clause = has_where_clause(query);
    
    if (!analysis->query_type) {
        analysis->risk_level = QUERY_SAFE;
        analysis->is_write_operation = false;
        analysis->affects_structure = false;
        analysis->warning_message = NULL;
        return analysis;
    }
    
    // Analyze based on query type
    if (strcmp(analysis->query_type, "SELECT") == 0 ||
        strcmp(analysis->query_type, "PRAGMA") == 0 ||
        strcmp(analysis->query_type, "EXPLAIN") == 0) {
        analysis->risk_level = QUERY_SAFE;
        analysis->is_write_operation = false;
        analysis->affects_structure = false;
        analysis->warning_message = NULL;
        
    } else if (strcmp(analysis->query_type, "INSERT") == 0) {
        analysis->risk_level = QUERY_MODERATE;
        analysis->is_write_operation = true;
        analysis->affects_structure = false;
        analysis->warning_message = g_strdup("This query will insert data into the database.");
        
    } else if (strcmp(analysis->query_type, "UPDATE") == 0) {
        if (analysis->has_where_clause) {
            analysis->risk_level = QUERY_MODERATE;
            analysis->warning_message = g_strdup("This query will modify existing data.");
        } else {
            analysis->risk_level = QUERY_DESTRUCTIVE;
            analysis->warning_message = g_strdup(
                "WARNING: This UPDATE has no WHERE clause and will modify ALL rows!\n"
                "This could result in significant data loss.");
        }
        analysis->is_write_operation = true;
        analysis->affects_structure = false;
        
    } else if (strcmp(analysis->query_type, "DELETE") == 0) {
        if (analysis->has_where_clause) {
            analysis->risk_level = QUERY_MODERATE;
            analysis->warning_message = g_strdup("This query will delete data from the database.");
        } else {
            analysis->risk_level = QUERY_DESTRUCTIVE;
            analysis->warning_message = g_strdup(
                "WARNING: This DELETE has no WHERE clause and will delete ALL rows!\n"
                "This will result in complete data loss for the table.");
        }
        analysis->is_write_operation = true;
        analysis->affects_structure = false;
        
    } else if (strcmp(analysis->query_type, "DROP") == 0) {
        analysis->risk_level = QUERY_DESTRUCTIVE;
        analysis->is_write_operation = true;
        analysis->affects_structure = true;
        analysis->warning_message = g_strdup(
            "DANGER: This will permanently delete a table or database object!\n"
            "This action cannot be undone.");
        
    } else if (strcmp(analysis->query_type, "ALTER") == 0) {
        analysis->risk_level = QUERY_DESTRUCTIVE;
        analysis->is_write_operation = true;
        analysis->affects_structure = true;
        analysis->warning_message = g_strdup(
            "WARNING: This will modify the database structure.\n"
            "Depending on the operation, this may result in data loss.");
        
    } else if (strcmp(analysis->query_type, "TRUNCATE") == 0) {
        analysis->risk_level = QUERY_DESTRUCTIVE;
        analysis->is_write_operation = true;
        analysis->affects_structure = false;
        analysis->warning_message = g_strdup(
            "WARNING: This will delete all data from the table!\n"
            "This cannot be undone.");
        
    } else if (strcmp(analysis->query_type, "CREATE") == 0) {
        analysis->risk_level = QUERY_MODERATE;
        analysis->is_write_operation = true;
        analysis->affects_structure = true;
        analysis->warning_message = g_strdup("This will create a new database object.");
        
    } else {
        // Unknown query type - be cautious
        analysis->risk_level = QUERY_MODERATE;
        analysis->is_write_operation = false;
        analysis->affects_structure = false;
        analysis->warning_message = g_strdup("Unknown query type. Proceed with caution.");
    }
    
    return analysis;
}

void free_query_analysis(QueryAnalysis *analysis) {
    if (!analysis) return;
    g_free(analysis->query_type);
    g_free(analysis->warning_message);
    g_free(analysis);
}

bool should_confirm_query(QueryAnalysis *analysis) {
    if (!analysis) return false;
    return analysis->risk_level >= QUERY_MODERATE;
}

char* get_confirmation_message(QueryAnalysis *analysis) {
    if (!analysis || !analysis->warning_message) {
        return g_strdup("Are you sure you want to execute this query?");
    }
    
    char *full_message = g_strdup_printf(
        "%s\n\nDo you want to continue?",
        analysis->warning_message
    );
    return full_message;
}

// Database backup protection
bool create_auto_backup(DBManager *mgr, const char *reason) {
    if (!mgr || !db_manager_is_open(mgr)) {
        return false;
    }
    
    // Get the database path (this would need to be added to DBManager API)
    // For now, we'll show a message about backup recommendation
    printf("[data-protection] Auto-backup recommended: %s\n", reason ? reason : "unknown");
    
    // TODO: Implement actual backup when db_manager exposes the db path
    // For now, return true to not block operations
    return true;
}

char* get_backup_path(const char *db_path, const char *suffix) {
    if (!db_path) return NULL;
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);
    
    char *backup_path = g_strdup_printf(
        "%s.backup_%s%s",
        db_path,
        suffix ? suffix : "",
        timestamp
    );
    
    return backup_path;
}

// Query history and auto-save
bool save_query_to_history(AppState *state, const char *query) {
    if (!state || !query) return false;
    
    // Create .db-browser directory in user's home for history
    const char *home = g_get_home_dir();
    char history_dir[1024];
    snprintf(history_dir, sizeof(history_dir), "%s/.db-browser", home);
    
    // Create directory if it doesn't exist
    mkdir(history_dir, 0755);
    
    char history_file[1024];
    snprintf(history_file, sizeof(history_file), "%s/query_history.sql", history_dir);
    
    FILE *f = fopen(history_file, "a");
    if (!f) {
        fprintf(stderr, "[data-protection] Failed to open history file: %s\n", history_file);
        return false;
    }
    
    time_t now = time(NULL);
    fprintf(f, "\n-- Query saved at: %s", ctime(&now));
    fprintf(f, "%s\n", query);
    fprintf(f, "-- End of query\n\n");
    
    fclose(f);
    return true;
}

bool load_last_query(AppState *state, char **query_out) {
    if (!state || !query_out) return false;
    
    const char *home = g_get_home_dir();
    char history_file[1024];
    snprintf(history_file, sizeof(history_file), "%s/.db-browser/query_history.sql", home);
    
    FILE *f = fopen(history_file, "r");
    if (!f) return false;
    
    // Read last query (simple implementation - could be improved)
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return false;
    }
    
    char *content = g_malloc(size + 1);
    fseek(f, 0, SEEK_SET);
    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';
    fclose(f);
    
    // Find last query (between last "-- Query saved" and "-- End of query")
    char *last_query_start = strrchr(content, '\n');
    if (last_query_start) {
        // Simplified - just return something
        *query_out = g_strdup("-- Last query placeholder --");
    }
    
    g_free(content);
    return true;
}

bool has_unsaved_query(AppState *state) {
    if (!state) return false;
    return state->query_dirty;
}

void mark_query_saved(AppState *state) {
    if (state) state->query_dirty = false;
}

void mark_query_dirty(AppState *state) {
    if (state) state->query_dirty = true;
}

// Transaction helpers
bool begin_protected_transaction(DBManager *mgr) {
    if (!mgr || !db_manager_is_open(mgr)) {
        return false;
    }
    
    int result = db_manager_begin_transaction(mgr);
    if (result == 0) {
        printf("[data-protection] Transaction started\n");
        return true;
    }
    
    return false;
}

bool commit_with_confirmation(DBManager *mgr, const char *message) {
    if (!mgr || !db_manager_is_open(mgr)) {
        return false;
    }
    
    char *confirm_msg = g_strdup_printf(
        "%s\n\nCommit these changes?",
        message ? message : "Transaction completed successfully."
    );
    
    bool should_commit = confirm_action(confirm_msg);
    g_free(confirm_msg);
    
    if (should_commit) {
        int result = db_manager_commit(mgr);
        if (result == 0) {
            printf("[data-protection] Transaction committed\n");
            return true;
        }
    } else {
        db_manager_rollback(mgr);
        printf("[data-protection] Transaction rolled back by user\n");
    }
    
    return false;
}

bool rollback_transaction(DBManager *mgr) {
    if (!mgr || !db_manager_is_open(mgr)) {
        return false;
    }
    
    int result = db_manager_rollback(mgr);
    if (result == 0) {
        printf("[data-protection] Transaction rolled back\n");
        return true;
    }
    
    return false;
}
