#ifndef EXPORT_H
#define EXPORT_H

#include "db_manager.h"

/* Export formats */
typedef enum {
    EXPORT_CSV,
    EXPORT_JSON,
    EXPORT_SQL,
    EXPORT_HTML,
    EXPORT_XML
} ExportFormat;

/* Export options */
typedef struct {
    ExportFormat format;
    bool include_header;
    char delimiter;
    bool pretty_print;
    bool include_schema;
    char *table_name;
    char *where_clause;
    int limit;
} ExportOptions;

/* Export result */
typedef struct {
    bool success;
    int rows_exported;
    char *output_data;
    char *error_message;
} ExportResult;

/* Export API */
ExportOptions* export_options_create(ExportFormat format);
void export_options_destroy(ExportOptions *opts);

/* Export to file */
ExportResult* export_to_file(DBManager *mgr, const char *file_path, ExportOptions *opts);
ExportResult* export_csv(DBManager *mgr, const char *file_path, ExportOptions *opts);
ExportResult* export_json(DBManager *mgr, const char *file_path, ExportOptions *opts);
ExportResult* export_sql(DBManager *mgr, const char *file_path, ExportOptions *opts);

/* Export to string */
char* export_csv_string(DBManager *mgr, ExportOptions *opts);
char* export_json_string(DBManager *mgr, ExportOptions *opts);
char* export_sql_string(DBManager *mgr, ExportOptions *opts);

/* Backup */
int backup_database(DBManager *mgr, const char *backup_path);

/* Cleanup */
void export_result_destroy(ExportResult *result);

#endif /* EXPORT_H */
