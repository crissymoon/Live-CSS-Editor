#include "db_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Create database manager */
DBManager* db_manager_create(const char *db_path, bool read_only) {
    DBManager *mgr = (DBManager*)malloc(sizeof(DBManager));
    if (!mgr) {
        fprintf(stderr, "[db_manager] malloc failed: out of memory\n");
        return NULL;
    }

    mgr->db = NULL;
    mgr->db_path = strdup(db_path);
    mgr->read_only = read_only;
    mgr->transaction_depth = 0;
    mgr->last_error = NULL;

    return mgr;
}

/* Destroy database manager */
void db_manager_destroy(DBManager *mgr) {
    if (!mgr) return;

    if (mgr->db) {
        sqlite3_close(mgr->db);
    }

    free(mgr->db_path);
    free(mgr->last_error);
    free(mgr);
}

/* Open database */
int db_manager_open(DBManager *mgr) {
    if (!mgr) return SQLITE_ERROR;

    int flags = mgr->read_only ?
                SQLITE_OPEN_READONLY :
                (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    int rc = sqlite3_open_v2(mgr->db_path, &mgr->db, flags, NULL);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = strdup(sqlite3_errmsg(mgr->db));
        fprintf(stderr, "[db_manager] open failed (%s): %s\n", mgr->db_path, mgr->last_error);
        return rc;
    }

    return SQLITE_OK;
}

/* Close database */
int db_manager_close(DBManager *mgr) {
    if (!mgr || !mgr->db) return SQLITE_ERROR;

    int rc = sqlite3_close(mgr->db);
    if (rc == SQLITE_OK) {
        mgr->db = NULL;
    } else {
        fprintf(stderr, "[db_manager] close failed: %s\n", sqlite3_errmsg(mgr->db));
    }

    return rc;
}

/* Check if database is open */
bool db_manager_is_open(DBManager *mgr) {
    return mgr && mgr->db != NULL;
}

/* Begin transaction */
int db_manager_begin_transaction(DBManager *mgr) {
    if (!mgr || !mgr->db || mgr->read_only) return SQLITE_ERROR;

    if (mgr->transaction_depth == 0) {
        char *err = NULL;
        int rc = sqlite3_exec(mgr->db, "BEGIN TRANSACTION", NULL, NULL, &err);
        if (rc != SQLITE_OK) {
            if (mgr->last_error) free(mgr->last_error);
            mgr->last_error = err ? strdup(err) : NULL;
            fprintf(stderr, "[db_manager] BEGIN TRANSACTION failed: %s\n", err ? err : "unknown error");
            sqlite3_free(err);
            return rc;
        }
    }

    mgr->transaction_depth++;
    return SQLITE_OK;
}

/* Commit transaction */
int db_manager_commit(DBManager *mgr) {
    if (!mgr || !mgr->db || mgr->transaction_depth == 0) return SQLITE_ERROR;

    mgr->transaction_depth--;

    if (mgr->transaction_depth == 0) {
        char *err = NULL;
        int rc = sqlite3_exec(mgr->db, "COMMIT", NULL, NULL, &err);
        if (rc != SQLITE_OK) {
            if (mgr->last_error) free(mgr->last_error);
            mgr->last_error = err ? strdup(err) : NULL;
            fprintf(stderr, "[db_manager] COMMIT failed: %s\n", err ? err : "unknown error");
            sqlite3_free(err);
            return rc;
        }
    }

    return SQLITE_OK;
}

/* Rollback transaction */
int db_manager_rollback(DBManager *mgr) {
    if (!mgr || !mgr->db || mgr->transaction_depth == 0) return SQLITE_ERROR;

    mgr->transaction_depth = 0;

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, "ROLLBACK", NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        fprintf(stderr, "[db_manager] ROLLBACK failed: %s\n", err ? err : "unknown error");
        sqlite3_free(err);
        return rc;
    }

    return SQLITE_OK;
}

/* Callback for getting tables */
static int get_tables_callback(void *data, int argc, char **argv, char **col_names) {
    TableInfo **head = (TableInfo**)data;

    if (argc > 0 && argv[0]) {
        TableInfo *info = (TableInfo*)malloc(sizeof(TableInfo));
        info->name = strdup(argv[0]);
        info->column_count = 0;
        info->row_count = 0;
        info->column_names = NULL;
        info->column_types = NULL;
        info->next = *head;
        *head = info;
    }

    return 0;
}

/* Get list of tables */
TableInfo* db_manager_get_tables(DBManager *mgr) {
    if (!mgr || !mgr->db) return NULL;

    TableInfo *tables = NULL;
    const char *query = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name";

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, query, get_tables_callback, &tables, &err);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        fprintf(stderr, "[db_manager] get_tables failed: %s\n", err ? err : "unknown error");
        sqlite3_free(err);
        return NULL;
    }

    // Get row counts for each table
    TableInfo *current = tables;
    while (current) {
        current->row_count = db_manager_get_table_row_count(mgr, current->name);
        current = current->next;
    }

    return tables;
}

/* Callback for getting columns */
static int get_columns_callback(void *data, int argc, char **argv, char **col_names) {
    ColumnInfo **head = (ColumnInfo**)data;

    if (argc >= 6) {
        ColumnInfo *info = (ColumnInfo*)malloc(sizeof(ColumnInfo));
        info->name = argv[1] ? strdup(argv[1]) : NULL;
        info->type = argv[2] ? strdup(argv[2]) : NULL;
        info->not_null = argv[3] && strcmp(argv[3], "1") == 0;
        info->default_value = argv[4] ? strdup(argv[4]) : NULL;
        info->primary_key = argv[5] && strcmp(argv[5], "1") == 0;
        info->unique = false;  // Would need separate query
        info->next = *head;
        *head = info;
    }

    return 0;
}

/* Get columns for a table */
ColumnInfo* db_manager_get_columns(DBManager *mgr, const char *table_name) {
    if (!mgr || !mgr->db || !table_name) return NULL;

    ColumnInfo *columns = NULL;
    char query[512];
    snprintf(query, sizeof(query), "PRAGMA table_info(%s)", table_name);

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, query, get_columns_callback, &columns, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[db_manager] get_columns failed for table '%s': %s\n", table_name, err ? err : "unknown error");
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        sqlite3_free(err);
        return NULL;
    }

    // Reverse list (callback builds in reverse order)
    ColumnInfo *prev = NULL;
    ColumnInfo *current = columns;
    ColumnInfo *next = NULL;
    while (current) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }

    return prev;
}

/* Create table */
int db_manager_create_table(DBManager *mgr, const char *table_name, ColumnInfo *columns) {
    if (!mgr || !mgr->db || !table_name || !columns || mgr->read_only) {
        return SQLITE_ERROR;
    }

    if (!db_manager_is_valid_identifier(table_name)) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = strdup("Invalid table name");
        fprintf(stderr, "[db_manager] create_table rejected invalid identifier: '%s'\n", table_name);
        return SQLITE_ERROR;
    }

    // Build CREATE TABLE statement
    char query[4096] = {0};
    int offset = snprintf(query, sizeof(query), "CREATE TABLE %s (", table_name);

    ColumnInfo *col = columns;
    bool first = true;

    while (col && offset < sizeof(query) - 100) {
        if (!first) {
            offset += snprintf(query + offset, sizeof(query) - offset, ", ");
        }
        first = false;

        offset += snprintf(query + offset, sizeof(query) - offset, "%s %s",
                          col->name, col->type);

        if (col->primary_key) {
            offset += snprintf(query + offset, sizeof(query) - offset, " PRIMARY KEY");
        }
        if (col->not_null) {
            offset += snprintf(query + offset, sizeof(query) - offset, " NOT NULL");
        }
        if (col->unique) {
            offset += snprintf(query + offset, sizeof(query) - offset, " UNIQUE");
        }
        if (col->default_value) {
            offset += snprintf(query + offset, sizeof(query) - offset,
                             " DEFAULT %s", col->default_value);
        }

        col = col->next;
    }

    snprintf(query + offset, sizeof(query) - offset, ")");

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        fprintf(stderr, "[db_manager] create_table '%s' failed: %s\n", table_name, err ? err : "unknown error");
        sqlite3_free(err);
    }

    return rc;
}

/* Drop table */
int db_manager_drop_table(DBManager *mgr, const char *table_name) {
    if (!mgr || !mgr->db || !table_name || mgr->read_only) {
        return SQLITE_ERROR;
    }

    char query[256];
    snprintf(query, sizeof(query), "DROP TABLE %s", table_name);

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        fprintf(stderr, "[db_manager] drop_table '%s' failed: %s\n", table_name, err ? err : "unknown error");
        sqlite3_free(err);
    }

    return rc;
}

/* Rename table */
int db_manager_rename_table(DBManager *mgr, const char *old_name, const char *new_name) {
    if (!mgr || !mgr->db || !old_name || !new_name || mgr->read_only) {
        return SQLITE_ERROR;
    }

    char query[512];
    snprintf(query, sizeof(query), "ALTER TABLE %s RENAME TO %s", old_name, new_name);

    char *err = NULL;
    int rc = sqlite3_exec(mgr->db, query, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = err ? strdup(err) : NULL;
        fprintf(stderr, "[db_manager] rename_table '%s' -> '%s' failed: %s\n", old_name, new_name, err ? err : "unknown error");
        sqlite3_free(err);
    }

    return rc;
}

/* Execute query and return results */
QueryResult* db_manager_execute_query(DBManager *mgr, const char *query) {
    if (!mgr || !mgr->db || !query) return NULL;

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mgr->db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        if (mgr->last_error) free(mgr->last_error);
        mgr->last_error = strdup(sqlite3_errmsg(mgr->db));
        fprintf(stderr, "[db_manager] execute_query prepare failed: %s\nQuery: %.200s\n", mgr->last_error, query);
        return NULL;
    }

    QueryResult *result = (QueryResult*)malloc(sizeof(QueryResult));
    result->column_count = sqlite3_column_count(stmt);
    result->row_count = 0;
    result->error = NULL;

    // Get column names
    result->column_names = (char**)malloc(result->column_count * sizeof(char*));
    for (int i = 0; i < result->column_count; i++) {
        const char *name = sqlite3_column_name(stmt, i);
        result->column_names[i] = name ? strdup(name) : strdup("");
    }

    // Count rows first
    int max_rows = 1000;  // Limit for safety
    while (sqlite3_step(stmt) == SQLITE_ROW && result->row_count < max_rows) {
        result->row_count++;
    }

    // Reset and fetch data
    sqlite3_reset(stmt);
    result->data = (char***)malloc(result->row_count * sizeof(char**));

    int row = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && row < result->row_count) {
        result->data[row] = (char**)malloc(result->column_count * sizeof(char*));

        for (int col = 0; col < result->column_count; col++) {
            const unsigned char *text = sqlite3_column_text(stmt, col);
            result->data[row][col] = text ? strdup((const char*)text) : strdup("[NULL]");
        }

        row++;
    }

    sqlite3_finalize(stmt);
    return result;
}

/* Get table row count */
long db_manager_get_table_row_count(DBManager *mgr, const char *table_name) {
    if (!mgr || !mgr->db || !table_name) return -1;

    char query[256];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM %s", table_name);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mgr->db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[db_manager] get_table_row_count prepare failed for '%s': %s\n", table_name, sqlite3_errmsg(mgr->db));
        return -1;
    }

    long count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

/* Get database size */
long db_manager_get_database_size(DBManager *mgr) {
    if (!mgr || !mgr->db) return -1;

    const char *query = "SELECT page_count * page_size FROM pragma_page_count(), pragma_page_size()";

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(mgr->db, query, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[db_manager] get_database_size prepare failed: %s\n", sqlite3_errmsg(mgr->db));
        return -1;
    }

    long size = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        size = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return size;
}

/* Get last error */
char* db_manager_get_last_error(DBManager *mgr) {
    if (!mgr) return NULL;
    return mgr->last_error;
}

/* Validate identifier (table/column name) */
bool db_manager_is_valid_identifier(const char *name) {
    if (!name || !*name) return false;

    // First character must be letter or underscore
    if (!isalpha(name[0]) && name[0] != '_') return false;

    // Remaining characters must be alphanumeric or underscore
    for (const char *p = name + 1; *p; p++) {
        if (!isalnum(*p) && *p != '_') return false;
    }

    // Check length
    if (strlen(name) > 64) return false;

    // Check reserved words
    const char *reserved[] = {
        "SELECT", "FROM", "WHERE", "INSERT", "UPDATE", "DELETE",
        "CREATE", "DROP", "ALTER", "TABLE", "INDEX", NULL
    };

    for (int i = 0; reserved[i]; i++) {
        if (strcasecmp(name, reserved[i]) == 0) return false;
    }

    return true;
}

/* Validate data type */
bool db_manager_is_valid_type(const char *type) {
    if (!type) return false;

    const char *valid_types[] = {
        "INTEGER", "TEXT", "REAL", "BLOB", "NUMERIC", NULL
    };

    for (int i = 0; valid_types[i]; i++) {
        if (strcasecmp(type, valid_types[i]) == 0) return true;
    }

    return false;
}

/* Free table info */
void db_manager_free_table_info(TableInfo *info) {
    while (info) {
        TableInfo *next = info->next;
        free(info->name);

        if (info->column_names) {
            for (int i = 0; i < info->column_count; i++) {
                free(info->column_names[i]);
            }
            free(info->column_names);
        }

        if (info->column_types) {
            for (int i = 0; i < info->column_count; i++) {
                free(info->column_types[i]);
            }
            free(info->column_types);
        }

        free(info);
        info = next;
    }
}

/* Free column info */
void db_manager_free_column_info(ColumnInfo *info) {
    while (info) {
        ColumnInfo *next = info->next;
        free(info->name);
        free(info->type);
        free(info->default_value);
        free(info);
        info = next;
    }
}

/* Free query result */
void db_manager_free_query_result(QueryResult *result) {
    if (!result) return;

    if (result->column_names) {
        for (int i = 0; i < result->column_count; i++) {
            free(result->column_names[i]);
        }
        free(result->column_names);
    }

    if (result->data) {
        for (int i = 0; i < result->row_count; i++) {
            if (result->data[i]) {
                for (int j = 0; j < result->column_count; j++) {
                    free(result->data[i][j]);
                }
                free(result->data[i]);
            }
        }
        free(result->data);
    }

    free(result->error);
    free(result);
}
