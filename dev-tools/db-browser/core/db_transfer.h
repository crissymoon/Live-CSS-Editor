#ifndef DB_TRANSFER_H
#define DB_TRANSFER_H

#include <stdbool.h>
#include <time.h>

/* Database storage and transfer API */

typedef struct {
    char *name;              // Database name (without extension)
    char *local_path;        // Full path to local database
    char *encrypted_path;    // Full path to encrypted version
    size_t size;             // File size in bytes
    time_t modified;         // Last modified timestamp
    bool is_encrypted;       // Whether database is encrypted
} DatabaseInfo;

/* Initialize database storage directory */
bool db_transfer_init(const char *storage_dir);

/* Get the configured storage directory */
const char* db_transfer_get_storage_dir(void);

/* List all databases in storage */
DatabaseInfo** db_transfer_list_databases(int *count);
void db_transfer_free_database_list(DatabaseInfo **list, int count);

/* Import database to storage (with optional encryption) */
bool db_transfer_import(const char *source_path,
                       const char *db_name,
                       const char *password,  // NULL for no encryption
                       bool keep_unencrypted);

/* Export database from storage (decrypt if needed) */
bool db_transfer_export(const char *db_name,
                       const char *dest_path,
                       const char *password);  // NULL if not encrypted

/* Open database from storage (decrypt to temp if encrypted) */
char* db_transfer_open_for_use(const char *db_name,
                               const char *password);  // Returns temp path

/* Save changes back to storage (re-encrypt if needed) */
bool db_transfer_save_changes(const char *db_name,
                              const char *temp_path,
                              const char *password);

/* Delete database from storage */
bool db_transfer_delete(const char *db_name, bool delete_encrypted_too);

/* Encrypt existing database in storage */
bool db_transfer_encrypt_database(const char *db_name,
                                  const char *password,
                                  bool delete_unencrypted);

/* Decrypt database in storage */
bool db_transfer_decrypt_database(const char *db_name,
                                  const char *password,
                                  bool delete_encrypted);

/* Create backup of database */
bool db_transfer_create_backup(const char *db_name,
                               const char *backup_suffix);

/* Prepare database for remote transfer (ensure encrypted) */
char* db_transfer_prepare_for_remote(const char *db_name,
                                     const char *password);

/* Receive database from remote location (decrypt after transfer) */
bool db_transfer_receive_from_remote(const char *encrypted_path,
                                     const char *db_name,
                                     const char *password);

/* Verify database integrity */
bool db_transfer_verify_integrity(const char *db_name);

/* Get database info */
DatabaseInfo* db_transfer_get_info(const char *db_name);
void db_transfer_free_info(DatabaseInfo *info);

/* Utility: Generate secure random password */
char* db_transfer_generate_password(int length);

#endif // DB_TRANSFER_H
