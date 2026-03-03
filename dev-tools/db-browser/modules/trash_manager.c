#include "trash_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <libgen.h>
#include <unistd.h>

static bool ensure_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) != 0) {
            return false;
        }
    }
    return true;
}

TrashManager* trash_manager_init(const char *source_db_path) {
    if (!source_db_path) return NULL;

    TrashManager *manager = malloc(sizeof(TrashManager));
    if (!manager) return NULL;

    char *db_path_copy = strdup(source_db_path);
    char *db_dir = dirname(db_path_copy);
    
    manager->trash_dir = malloc(strlen(db_dir) + 20);
    sprintf(manager->trash_dir, "%s/trash", db_dir);
    free(db_path_copy);

    if (!ensure_directory(manager->trash_dir)) {
        free(manager->trash_dir);
        free(manager);
        return NULL;
    }

    manager->trash_db_path = malloc(strlen(manager->trash_dir) + 30);
    sprintf(manager->trash_db_path, "%s/trash.db", manager->trash_dir);

    if (sqlite3_open(manager->trash_db_path, &manager->trash_db) != SQLITE_OK) {
        free(manager->trash_dir);
        free(manager->trash_db_path);
        free(manager);
        return NULL;
    }

    const char *create_table_sql = 
        "CREATE TABLE IF NOT EXISTS deleted_rows ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  source_database TEXT NOT NULL,"
        "  table_name TEXT NOT NULL,"
        "  original_rowid INTEGER NOT NULL,"
        "  column_names TEXT NOT NULL,"
        "  column_values TEXT NOT NULL,"
        "  deleted_at TEXT NOT NULL,"
        "  deleted_by TEXT"
        ");"
        "CREATE TABLE IF NOT EXISTS dropped_tables ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  source_database TEXT NOT NULL,"
        "  table_name TEXT NOT NULL,"
        "  create_sql TEXT NOT NULL,"
        "  row_count INTEGER NOT NULL,"
        "  data_sql TEXT,"
        "  dropped_at TEXT NOT NULL,"
        "  dropped_by TEXT"
        ");";

    char *err_msg = NULL;
    if (sqlite3_exec(manager->trash_db, create_table_sql, NULL, NULL, &err_msg) != SQLITE_OK) {
        if (err_msg) {
            sqlite3_free(err_msg);
        }
        sqlite3_close(manager->trash_db);
        free(manager->trash_dir);
        free(manager->trash_db_path);
        free(manager);
        return NULL;
    }

    return manager;
}

bool trash_manager_save_row(TrashManager *manager,
                             const char *source_db,
                             const char *table_name,
                             const char **column_names,
                             const char **values,
                             int column_count,
                             long rowid) {
    if (!manager || !source_db || !table_name || !column_names || !values) {
        return false;
    }

    char names_json[4096] = {0};
    char values_json[4096] = {0};
    int names_pos = 1;
    int values_pos = 1;

    names_json[0] = '[';
    values_json[0] = '[';

    for (int i = 0; i < column_count && names_pos < 4000 && values_pos < 4000; i++) {
        if (i > 0) {
            names_json[names_pos++] = ',';
            values_json[values_pos++] = ',';
        }
        
        int written = snprintf(names_json + names_pos, 4000 - names_pos, 
                               "\"%s\"", column_names[i] ? column_names[i] : "");
        if (written > 0) names_pos += written;
        
        const char *val = values[i] ? values[i] : "NULL";
        if (values[i]) {
            written = snprintf(values_json + values_pos, 4000 - values_pos, 
                              "\"%s\"", val);
        } else {
            written = snprintf(values_json + values_pos, 4000 - values_pos, "null");
        }
        if (written > 0) values_pos += written;
    }

    names_json[names_pos++] = ']';
    names_json[names_pos] = '\0';
    values_json[values_pos++] = ']';
    values_json[values_pos] = '\0';

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char username[256] = "unknown";
    getlogin_r(username, sizeof(username));

    const char *insert_sql = 
        "INSERT INTO deleted_rows (source_database, table_name, original_rowid, "
        "column_names, column_values, deleted_at, deleted_by) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(manager->trash_db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, source_db, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, table_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, rowid);
    sqlite3_bind_text(stmt, 4, names_json, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, values_json, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, timestamp, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, username, -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (result == SQLITE_DONE);
}

char** trash_manager_list_entries(TrashManager *manager,
                                   const char *source_db,
                                   const char *table_name,
                                   int *count) {
    if (!manager || !count) return NULL;

    *count = 0;

    const char *select_sql = 
        "SELECT id, deleted_at, deleted_by, original_rowid FROM deleted_rows "
        "WHERE source_database = ? AND table_name = ? ORDER BY id DESC;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(manager->trash_db, select_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, source_db, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, table_name, -1, SQLITE_TRANSIENT);

    char **entries = NULL;
    int capacity = 10;
    entries = malloc(sizeof(char*) * capacity);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            entries = realloc(entries, sizeof(char*) * capacity);
        }

        char entry[512];
        snprintf(entry, sizeof(entry), "ID: %lld | Deleted: %s | By: %s | Original Row: %lld",
                 sqlite3_column_int64(stmt, 0),
                 sqlite3_column_text(stmt, 1),
                 sqlite3_column_text(stmt, 2),
                 sqlite3_column_int64(stmt, 3));
        
        entries[*count] = strdup(entry);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool trash_manager_recover_row(TrashManager *manager,
                                const char *source_db,
                                const char *table_name,
                                long trash_id) {
    // Recovery would require opening the source database and inserting the row back
    // This is a placeholder implementation
    return false;
}

bool trash_manager_purge_entry(TrashManager *manager, long trash_id) {
    if (!manager) return false;

    const char *delete_sql = "DELETE FROM deleted_rows WHERE id = ?;";
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(manager->trash_db, delete_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int64(stmt, 1, trash_id);
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return (result == SQLITE_DONE);
}

bool trash_manager_save_table(TrashManager *manager,
                               const char *source_db,
                               const char *table_name,
                               const char *create_sql,
                               sqlite3 *source_db_handle) {
    if (!manager || !source_db || !table_name || !create_sql || !source_db_handle) {
        return false;
    }

    // Get row count
    char count_query[512];
    snprintf(count_query, sizeof(count_query), "SELECT COUNT(*) FROM \"%s\"", table_name);
    
    sqlite3_stmt *count_stmt;
    int row_count = 0;
    if (sqlite3_prepare_v2(source_db_handle, count_query, -1, &count_stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(count_stmt) == SQLITE_ROW) {
            row_count = sqlite3_column_int(count_stmt, 0);
        }
        sqlite3_finalize(count_stmt);
    }

    // Get all data as INSERT statements
    char select_query[512];
    snprintf(select_query, sizeof(select_query), "SELECT * FROM \"%s\"", table_name);
    
    sqlite3_stmt *data_stmt;
    char *data_sql = NULL;
    size_t data_sql_size = 0;
    size_t data_sql_capacity = 4096;
    data_sql = malloc(data_sql_capacity);
    if (!data_sql) return false;
    data_sql[0] = '\0';

    if (sqlite3_prepare_v2(source_db_handle, select_query, -1, &data_stmt, NULL) == SQLITE_OK) {
        int col_count = sqlite3_column_count(data_stmt);
        
        while (sqlite3_step(data_stmt) == SQLITE_ROW && data_sql_size < 1000000) {
            char insert_line[4096];
            int pos = snprintf(insert_line, sizeof(insert_line), "INSERT INTO \"%s\" VALUES(", table_name);
            
            for (int i = 0; i < col_count; i++) {
                if (i > 0) insert_line[pos++] = ',';
                
                int col_type = sqlite3_column_type(data_stmt, i);
                if (col_type == SQLITE_NULL) {
                    pos += snprintf(insert_line + pos, sizeof(insert_line) - pos, "NULL");
                } else if (col_type == SQLITE_INTEGER) {
                    pos += snprintf(insert_line + pos, sizeof(insert_line) - pos, "%lld",
                                   sqlite3_column_int64(data_stmt, i));
                } else if (col_type == SQLITE_FLOAT) {
                    pos += snprintf(insert_line + pos, sizeof(insert_line) - pos, "%g",
                                   sqlite3_column_double(data_stmt, i));
                } else {
                    const unsigned char *text = sqlite3_column_text(data_stmt, i);
                    pos += snprintf(insert_line + pos, sizeof(insert_line) - pos, "'");
                    for (const unsigned char *p = text; *p && pos < sizeof(insert_line) - 10; p++) {
                        if (*p == '\'') insert_line[pos++] = '\'';
                        insert_line[pos++] = *p;
                    }
                    pos += snprintf(insert_line + pos, sizeof(insert_line) - pos, "'");
                }
            }
            snprintf(insert_line + pos, sizeof(insert_line) - pos, ");\n");
            
            size_t line_len = strlen(insert_line);
            if (data_sql_size + line_len + 1 > data_sql_capacity) {
                data_sql_capacity *= 2;
                char *new_data = realloc(data_sql, data_sql_capacity);
                if (!new_data) {
                    free(data_sql);
                    sqlite3_finalize(data_stmt);
                    return false;
                }
                data_sql = new_data;
            }
            
            strcat(data_sql, insert_line);
            data_sql_size += line_len;
        }
        sqlite3_finalize(data_stmt);
    }

    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    char username[256] = "unknown";
    getlogin_r(username, sizeof(username));

    const char *insert_sql = 
        "INSERT INTO dropped_tables (source_database, table_name, create_sql, "
        "row_count, data_sql, dropped_at, dropped_by) VALUES (?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(manager->trash_db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        free(data_sql);
        return false;
    }

    sqlite3_bind_text(stmt, 1, source_db, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, table_name, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, create_sql, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, row_count);
    sqlite3_bind_text(stmt, 5, data_sql, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, timestamp, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, username, -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    free(data_sql);

    return (result == SQLITE_DONE);
}

char** trash_manager_list_tables(TrashManager *manager,
                                  const char *source_db,
                                  int *count) {
    if (!manager || !count) return NULL;

    *count = 0;

    const char *select_sql = 
        "SELECT id, table_name, row_count, dropped_at, dropped_by FROM dropped_tables "
        "WHERE source_database = ? ORDER BY id DESC;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(manager->trash_db, select_sql, -1, &stmt, NULL) != SQLITE_OK) {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, source_db, -1, SQLITE_TRANSIENT);

    char **entries = NULL;
    int capacity = 10;
    entries = malloc(sizeof(char*) * capacity);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (*count >= capacity) {
            capacity *= 2;
            entries = realloc(entries, sizeof(char*) * capacity);
        }

        char entry[512];
        snprintf(entry, sizeof(entry), "ID: %lld | Table: %s | Rows: %d | Dropped: %s | By: %s",
                 sqlite3_column_int64(stmt, 0),
                 sqlite3_column_text(stmt, 1),
                 sqlite3_column_int(stmt, 2),
                 sqlite3_column_text(stmt, 3),
                 sqlite3_column_text(stmt, 4));
        
        entries[*count] = strdup(entry);
        (*count)++;
    }

    sqlite3_finalize(stmt);
    return entries;
}

bool trash_manager_recover_table(TrashManager *manager,
                                  const char *source_db,
                                  const char *table_name,
                                  long trash_id) {
    // Recovery would require opening the source database and executing CREATE + INSERT statements
    // This is a placeholder implementation
    (void)manager;
    (void)source_db;
    (void)table_name;
    (void)trash_id;
    return false;
}

void trash_manager_close(TrashManager *manager) {
    if (!manager) return;

    if (manager->trash_db) {
        sqlite3_close(manager->trash_db);
    }

    free(manager->trash_dir);
    free(manager->trash_db_path);
    free(manager);
}
