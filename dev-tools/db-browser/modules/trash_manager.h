#ifndef TRASH_MANAGER_H
#define TRASH_MANAGER_H

#include <sqlite3.h>
#include <stdbool.h>

typedef struct {
    char *trash_dir;
    char *trash_db_path;
    sqlite3 *trash_db;
} TrashManager;

// Initialize trash system - creates trash directory and database
TrashManager* trash_manager_init(const char *source_db_path);

// Save deleted row to trash database
bool trash_manager_save_row(TrashManager *manager, 
                             const char *source_db,
                             const char *table_name,
                             const char **column_names,
                             const char **values,
                             int column_count,
                             long rowid);

// List all trash entries for a specific table
char** trash_manager_list_entries(TrashManager *manager,
                                   const char *source_db,
                                   const char *table_name,
                                   int *count);

// Recover a deleted row back to the source database
bool trash_manager_recover_row(TrashManager *manager,
                                const char *source_db,
                                const char *table_name,
                                long trash_id);

// Permanently delete trash entry
bool trash_manager_purge_entry(TrashManager *manager, long trash_id);

// Save dropped table to trash (schema + data)
bool trash_manager_save_table(TrashManager *manager,
                               const char *source_db,
                               const char *table_name,
                               const char *create_sql,
                               sqlite3 *source_db_handle);

// List all trashed tables
char** trash_manager_list_tables(TrashManager *manager,
                                  const char *source_db,
                                  int *count);

// Recover a dropped table back to the source database
bool trash_manager_recover_table(TrashManager *manager,
                                  const char *source_db,
                                  const char *table_name,
                                  long trash_id);

// Close and cleanup trash manager
void trash_manager_close(TrashManager *manager);

#endif // TRASH_MANAGER_H
