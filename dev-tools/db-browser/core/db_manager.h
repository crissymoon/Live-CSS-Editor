#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <sqlite3.h>
#include <stdbool.h>

/* Database manager structure */
typedef struct DBManager {
    sqlite3 *db;
    char *db_path;
    bool read_only;
    int transaction_depth;
    char *last_error;
} DBManager;

/* Table information */
typedef struct TableInfo {
    char *name;
    int column_count;
    int row_count;
    char **column_names;
    char **column_types;
    struct TableInfo *next;
} TableInfo;

/* Column information */
typedef struct ColumnInfo {
    char *name;
    char *type;
    bool not_null;
    bool primary_key;
    bool unique;
    char *default_value;
    struct ColumnInfo *next;
} ColumnInfo;

/* Query result */
typedef struct QueryResult {
    int column_count;
    int row_count;
    char **column_names;
    char ***data;  // [row][column]
    char *error;
} QueryResult;

/* Database Manager API */
DBManager* db_manager_create(const char *db_path, bool read_only);
void db_manager_destroy(DBManager *mgr);

/* Connection management */
int db_manager_open(DBManager *mgr);
int db_manager_close(DBManager *mgr);
bool db_manager_is_open(DBManager *mgr);

/* Transaction management */
int db_manager_begin_transaction(DBManager *mgr);
int db_manager_commit(DBManager *mgr);
int db_manager_rollback(DBManager *mgr);

/* Table operations */
TableInfo* db_manager_get_tables(DBManager *mgr);
ColumnInfo* db_manager_get_columns(DBManager *mgr, const char *table_name);
int db_manager_create_table(DBManager *mgr, const char *table_name, ColumnInfo *columns);
int db_manager_drop_table(DBManager *mgr, const char *table_name);
int db_manager_rename_table(DBManager *mgr, const char *old_name, const char *new_name);

/* Column operations */
int db_manager_add_column(DBManager *mgr, const char *table_name, ColumnInfo *column);
int db_manager_rename_column(DBManager *mgr, const char *table_name,
                             const char *old_name, const char *new_name);

/* Data operations */
QueryResult* db_manager_execute_query(DBManager *mgr, const char *query);
int db_manager_insert_row(DBManager *mgr, const char *table_name,
                          char **column_names, char **values, int count);
int db_manager_update_row(DBManager *mgr, const char *table_name,
                          char **column_names, char **values, int count,
                          const char *where_clause);
int db_manager_delete_rows(DBManager *mgr, const char *table_name, const char *where_clause);

/* Index operations */
int db_manager_create_index(DBManager *mgr, const char *index_name,
                            const char *table_name, char **columns, int count);
int db_manager_drop_index(DBManager *mgr, const char *index_name);

/* Database introspection */
int db_manager_get_table_count(DBManager *mgr);
long db_manager_get_table_row_count(DBManager *mgr, const char *table_name);
long db_manager_get_database_size(DBManager *mgr);
char* db_manager_get_schema(DBManager *mgr, const char *table_name);

/* Utility functions */
char* db_manager_get_last_error(DBManager *mgr);
void db_manager_free_table_info(TableInfo *info);
void db_manager_free_column_info(ColumnInfo *info);
void db_manager_free_query_result(QueryResult *result);

/* Validation */
bool db_manager_is_valid_identifier(const char *name);
bool db_manager_is_valid_type(const char *type);

#endif /* DB_MANAGER_H */
