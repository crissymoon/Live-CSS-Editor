#ifndef IMPORT_H
#define IMPORT_H

#include "db_manager.h"

/* Import formats */
typedef enum {
    IMPORT_CSV,
    IMPORT_JSON,
    IMPORT_SQL
} ImportFormat;

/* Import options */
typedef struct {
    ImportFormat format;
    bool has_header;
    char delimiter;
    bool skip_errors;
    int skip_rows;
    char *table_name;
    char **column_mapping;  // Maps file columns to table columns
    int column_count;
} ImportOptions;

/* Import result */
typedef struct {
    bool success;
    int rows_imported;
    int rows_skipped;
    char *error_message;
    char **errors;  // Array of error messages for skipped rows
    int error_count;
} ImportResult;

/* Import API */
ImportOptions* import_options_create(ImportFormat format);
void import_options_destroy(ImportOptions *opts);

/* Import from file */
ImportResult* import_from_file(DBManager *mgr, const char *file_path, ImportOptions *opts);
ImportResult* import_csv(DBManager *mgr, const char *file_path, ImportOptions *opts);
ImportResult* import_json(DBManager *mgr, const char *file_path, ImportOptions *opts);
ImportResult* import_sql(DBManager *mgr, const char *file_path);

/* Import from string */
ImportResult* import_csv_string(DBManager *mgr, const char *csv_data, ImportOptions *opts);
ImportResult* import_json_string(DBManager *mgr, const char *json_data, ImportOptions *opts);

/* Utilities */
char** parse_csv_line(const char *line, char delimiter, int *count);
char* detect_delimiter(const char *first_line);
bool validate_csv_header(const char **headers, int count);

/* Cleanup */
void import_result_destroy(ImportResult *result);

#endif /* IMPORT_H */
