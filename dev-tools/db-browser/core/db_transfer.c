#include "db_transfer.h"
#include "db_crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

static char storage_directory[1024] = {0};

/* uthash name-cache: head pointer for O(1) lookup by database name */
static DatabaseInfo *db_name_cache = NULL;

/* Initialize storage directory */
bool db_transfer_init(const char *storage_dir) {
    if (!storage_dir || strlen(storage_dir) == 0) {
        fprintf(stderr, "[db-transfer] Invalid storage directory\n");
        return false;
    }

    // Create directory if it doesn't exist
    struct stat st = {0};
    if (stat(storage_dir, &st) == -1) {
        if (mkdir(storage_dir, 0700) != 0) {
            fprintf(stderr, "[db-transfer] Failed to create storage directory: %s\n", 
                    strerror(errno));
            return false;
        }
    }

    strncpy(storage_directory, storage_dir, sizeof(storage_directory) - 1);
    storage_directory[sizeof(storage_directory) - 1] = '\0';

    printf("[db-transfer] Initialized storage: %s\n", storage_directory);
    return true;
}

const char* db_transfer_get_storage_dir(void) {
    return storage_directory[0] ? storage_directory : NULL;
}

/* Helper: construct full path in storage */
static char* make_storage_path(const char *db_name, const char *extension) {
    if (!storage_directory[0] || !db_name) {
        return NULL;
    }

    size_t path_len = strlen(storage_directory) + strlen(db_name) + 
                      (extension ? strlen(extension) : 0) + 10;
    char *path = malloc(path_len);
    if (!path) {
        return NULL;
    }

    if (extension) {
        snprintf(path, path_len, "%s/%s.%s", storage_directory, db_name, extension);
    } else {
        snprintf(path, path_len, "%s/%s", storage_directory, db_name);
    }

    return path;
}

/* Helper: check if file exists */
static bool file_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/* Helper: get file size */
static size_t get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

/* Helper: get file modification time */
static time_t get_file_mtime(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

/* List databases in storage */
DatabaseInfo** db_transfer_list_databases(int *count) {
    if (!count || !storage_directory[0]) {
        return NULL;
    }

    *count = 0;

    /* Discard any stale cache before rebuilding */
    HASH_CLEAR(hh, db_name_cache);

    DIR *dir = opendir(storage_directory);
    if (!dir) {
        fprintf(stderr, "[db-transfer] Failed to open storage directory: %s\n",
                strerror(errno));
        return NULL;
    }

    // First pass: count databases
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        size_t len = strlen(entry->d_name);
        if (len > 3 && strcmp(entry->d_name + len - 3, ".db") == 0) {
            (*count)++;
        }
    }

    if (*count == 0) {
        closedir(dir);
        return NULL;
    }

    // Allocate array
    DatabaseInfo **list = malloc(sizeof(DatabaseInfo*) * (*count));
    if (!list) {
        closedir(dir);
        *count = 0;
        return NULL;
    }

    // Second pass: populate list
    rewinddir(dir);
    int index = 0;
    while ((entry = readdir(dir)) != NULL && index < *count) {
        if (entry->d_name[0] == '.') continue;
        
        size_t len = strlen(entry->d_name);
        if (len > 3 && strcmp(entry->d_name + len - 3, ".db") == 0) {
            DatabaseInfo *info = malloc(sizeof(DatabaseInfo));
            if (!info) continue;

            memset(info, 0, sizeof(DatabaseInfo));

            // Extract name without extension
            char *name_copy = strdup(entry->d_name);
            char *dot = strrchr(name_copy, '.');
            if (dot) *dot = '\0';
            info->name = name_copy;

            // Build paths
            info->local_path = make_storage_path(info->name, "db");
            info->encrypted_path = make_storage_path(info->name, "db.enc");

            // Get file info
            if (file_exists(info->local_path)) {
                info->size = get_file_size(info->local_path);
                info->modified = get_file_mtime(info->local_path);
                info->is_encrypted = false;
            } else if (file_exists(info->encrypted_path)) {
                info->size = get_file_size(info->encrypted_path);
                info->modified = get_file_mtime(info->encrypted_path);
                info->is_encrypted = true;
                free(info->local_path);
                info->local_path = NULL;
            }

            list[index++] = info;

            /* Register in name-cache for O(1) lookups */
            HASH_ADD_STR(db_name_cache, name, info);
        }
    }

    closedir(dir);
    *count = index;
    return list;
}

void db_transfer_free_database_list(DatabaseInfo **list, int count) {
    if (!list) return;

    /* Detach hash table before freeing items to avoid dangling handles */
    HASH_CLEAR(hh, db_name_cache);

    for (int i = 0; i < count; i++) {
        if (list[i]) {
            free(list[i]->name);
            free(list[i]->local_path);
            free(list[i]->encrypted_path);
            free(list[i]);
        }
    }
    free(list);
}

/* O(1) database name lookup via uthash cache.
 * Returns a pointer into the last list returned by db_transfer_list_databases(),
 * or NULL if not found / cache is empty.
 * Do NOT free the returned pointer; it is owned by the cached list array.
 */
DatabaseInfo* db_transfer_find_by_name(const char *name) {
    if (!name) return NULL;
    DatabaseInfo *found = NULL;
    HASH_FIND_STR(db_name_cache, name, found);
    return found;
}

/* Explicitly invalidate the name cache without freeing the list items.
 * Call this if you manually free a list obtained from db_transfer_list_databases()
 * outside of db_transfer_free_database_list().
 */
void db_transfer_cache_clear(void) {
    HASH_CLEAR(hh, db_name_cache);
}

/* Import database to storage */
bool db_transfer_import(const char *source_path,
                       const char *db_name,
                       const char *password,
                       bool keep_unencrypted) {
    if (!source_path || !db_name || !storage_directory[0]) {
        return false;
    }

    if (!file_exists(source_path)) {
        fprintf(stderr, "[db-transfer] Source file not found: %s\n", source_path);
        return false;
    }

    // Copy to storage directory
    char *dest_path = make_storage_path(db_name, "db");
    if (!dest_path) {
        return false;
    }

    // Copy file
    FILE *src = fopen(source_path, "rb");
    if (!src) {
        fprintf(stderr, "[db-transfer] Failed to open source: %s\n", strerror(errno));
        free(dest_path);
        return false;
    }

    FILE *dst = fopen(dest_path, "wb");
    if (!dst) {
        fprintf(stderr, "[db-transfer] Failed to create destination: %s\n", strerror(errno));
        fclose(src);
        free(dest_path);
        return false;
    }

    char *buffer = malloc(8192);
    if (!buffer) {
        fclose(src);
        fclose(dst);
        unlink(dest_path);
        free(dest_path);
        return false;
    }
    
    size_t bytes;
    while ((bytes = fread(buffer, 1, 8192, src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            free(buffer);
            fclose(src);
            fclose(dst);
            unlink(dest_path);
            free(dest_path);
            return false;
        }
    }

    free(buffer);
    fclose(src);
    fclose(dst);

    printf("[db-transfer] Imported: %s -> %s\n", source_path, dest_path);

    // Encrypt if password provided
    if (password) {
        char *enc_path = make_storage_path(db_name, "db.enc");
        if (enc_path) {
            if (crypto_encrypt_file(dest_path, enc_path, password)) {
                printf("[db-transfer] Encrypted database: %s\n", enc_path);
                
                if (!keep_unencrypted) {
                    unlink(dest_path);
                    printf("[db-transfer] Removed unencrypted copy\n");
                }
            }
            free(enc_path);
        }
    }

    free(dest_path);
    return true;
}

/* Export database from storage */
bool db_transfer_export(const char *db_name,
                       const char *dest_path,
                       const char *password) {
    if (!db_name || !dest_path || !storage_directory[0]) {
        return false;
    }

    char *source_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");
    
    bool use_encrypted = false;

    // Check which version exists
    if (file_exists(source_path)) {
        use_encrypted = false;
    } else if (file_exists(enc_path)) {
        use_encrypted = true;
    } else {
        fprintf(stderr, "[db-transfer] Database not found: %s\n", db_name);
        free(source_path);
        free(enc_path);
        return false;
    }

    bool result = false;

    if (use_encrypted) {
        if (!password) {
            fprintf(stderr, "[db-transfer] Password required for encrypted database\n");
        } else {
            result = crypto_decrypt_file(enc_path, dest_path, password);
        }
    } else {
        // Simple copy
        FILE *src = fopen(source_path, "rb");
        if (src) {
            FILE *dst = fopen(dest_path, "wb");
            if (dst) {
                char *buffer = malloc(8192);
                if (buffer) {
                    size_t bytes;
                    result = true;
                    while ((bytes = fread(buffer, 1, 8192, src)) > 0) {
                        if (fwrite(buffer, 1, bytes, dst) != bytes) {
                            result = false;
                            break;
                        }
                    }
                    free(buffer);
                }
                fclose(dst);
            }
            fclose(src);
        }
    }

    free(source_path);
    free(enc_path);

    if (result) {
        printf("[db-transfer] Exported: %s -> %s\n", db_name, dest_path);
    }

    return result;
}

/* Open database for use */
char* db_transfer_open_for_use(const char *db_name, const char *password) {
    if (!db_name || !storage_directory[0]) {
        return NULL;
    }

    char *local_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");

    // If unencrypted version exists, use it directly
    if (file_exists(local_path)) {
        free(enc_path);
        return local_path;
    }

    // If encrypted version exists, decrypt to temp
    if (file_exists(enc_path)) {
        if (!password) {
            fprintf(stderr, "[db-transfer] Password required\n");
            free(local_path);
            free(enc_path);
            return NULL;
        }

        char temp_path[1024];
        snprintf(temp_path, sizeof(temp_path), "%s/%s.tmp", storage_directory, db_name);

        if (crypto_decrypt_file(enc_path, temp_path, password)) {
            free(local_path);
            free(enc_path);
            return strdup(temp_path);
        }
    }

    free(local_path);
    free(enc_path);
    return NULL;
}

/* Save changes back to storage */
bool db_transfer_save_changes(const char *db_name,
                              const char *temp_path,
                              const char *password) {
    if (!db_name || !temp_path || !storage_directory[0]) {
        return false;
    }

    char *enc_path = make_storage_path(db_name, "db.enc");
    
    // If password provided, re-encrypt
    if (password && enc_path) {
        bool result = crypto_encrypt_file(temp_path, enc_path, password);
        free(enc_path);
        return result;
    }

    // Otherwise, just copy to storage
    char *dest_path = make_storage_path(db_name, "db");
    if (!dest_path) {
        return false;
    }

    FILE *src = fopen(temp_path, "rb");
    if (!src) {
        free(dest_path);
        return false;
    }

    FILE *dst = fopen(dest_path, "wb");
    if (!dst) {
        fclose(src);
        free(dest_path);
        return false;
    }

    char *buffer = malloc(8192);
    if (!buffer) {
        fclose(src);
        fclose(dst);
        free(dest_path);
        return false;
    }
    
    size_t bytes;
    bool result = true;
    while ((bytes = fread(buffer, 1, 8192, src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            result = false;
            break;
        }
    }

    free(buffer);
    fclose(src);
    fclose(dst);
    free(dest_path);

    return result;
}

/* Delete database from storage */
bool db_transfer_delete(const char *db_name, bool delete_encrypted_too) {
    if (!db_name || !storage_directory[0]) {
        return false;
    }

    char *local_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");

    bool deleted_any = false;

    if (local_path && file_exists(local_path)) {
        if (unlink(local_path) == 0) {
            deleted_any = true;
            printf("[db-transfer] Deleted: %s\n", local_path);
        }
    }

    if (delete_encrypted_too && enc_path && file_exists(enc_path)) {
        if (unlink(enc_path) == 0) {
            deleted_any = true;
            printf("[db-transfer] Deleted encrypted: %s\n", enc_path);
        }
    }

    free(local_path);
    free(enc_path);

    return deleted_any;
}

/* Encrypt existing database */
bool db_transfer_encrypt_database(const char *db_name,
                                  const char *password,
                                  bool delete_unencrypted) {
    if (!db_name || !password || !storage_directory[0]) {
        return false;
    }

    char *local_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");

    if (!file_exists(local_path)) {
        fprintf(stderr, "[db-transfer] Database not found: %s\n", db_name);
        free(local_path);
        free(enc_path);
        return false;
    }

    bool result = crypto_encrypt_file(local_path, enc_path, password);

    if (result && delete_unencrypted) {
        unlink(local_path);
        printf("[db-transfer] Removed unencrypted version\n");
    }

    free(local_path);
    free(enc_path);

    return result;
}

/* Decrypt database in storage */
bool db_transfer_decrypt_database(const char *db_name,
                                  const char *password,
                                  bool delete_encrypted) {
    if (!db_name || !password || !storage_directory[0]) {
        return false;
    }

    char *local_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");

    if (!file_exists(enc_path)) {
        fprintf(stderr, "[db-transfer] Encrypted database not found: %s\n", db_name);
        free(local_path);
        free(enc_path);
        return false;
    }

    bool result = crypto_decrypt_file(enc_path, local_path, password);

    if (result && delete_encrypted) {
        unlink(enc_path);
        printf("[db-transfer] Removed encrypted version\n");
    }

    free(local_path);
    free(enc_path);

    return result;
}

/* Prepare for remote transfer */
char* db_transfer_prepare_for_remote(const char *db_name, const char *password) {
    if (!db_name || !password || !storage_directory[0]) {
        return NULL;
    }

    char *enc_path = make_storage_path(db_name, "db.enc");
    
    // If already encrypted, return that path
    if (file_exists(enc_path)) {
        return enc_path;
    }

    // Otherwise, encrypt it
    char *local_path = make_storage_path(db_name, "db");
    if (file_exists(local_path)) {
        if (crypto_encrypt_file(local_path, enc_path, password)) {
            free(local_path);
            return enc_path;
        }
    }

    free(local_path);
    free(enc_path);
    return NULL;
}

/* Receive from remote */
bool db_transfer_receive_from_remote(const char *encrypted_path,
                                     const char *db_name,
                                     const char *password) {
    if (!encrypted_path || !db_name || !password || !storage_directory[0]) {
        return false;
    }

    char *dest_enc_path = make_storage_path(db_name, "db.enc");
    if (!dest_enc_path) {
        return false;
    }

    // Copy encrypted file to storage
    FILE *src = fopen(encrypted_path, "rb");
    if (!src) {
        free(dest_enc_path);
        return false;
    }

    FILE *dst = fopen(dest_enc_path, "wb");
    if (!dst) {
        fclose(src);
        free(dest_enc_path);
        return false;
    }

    char *buffer = malloc(8192);
    if (!buffer) {
        fclose(src);
        fclose(dst);
        unlink(dest_enc_path);
        free(dest_enc_path);
        return false;
    }
    
    size_t bytes;
    bool copy_ok = true;
    while ((bytes = fread(buffer, 1, 8192, src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            copy_ok = false;
            break;
        }
    }

    free(buffer);
    fclose(src);
    fclose(dst);

    if (!copy_ok) {
        unlink(dest_enc_path);
        free(dest_enc_path);
        return false;
    }

    printf("[db-transfer] Received encrypted database: %s\n", dest_enc_path);
    free(dest_enc_path);
    return true;
}

/* Generate secure random password */
char* db_transfer_generate_password(int length) {
    if (length < 8 || length > 128) {
        length = 32;  // Default
    }

    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
    size_t charset_len = strlen(charset);

    char *password = malloc(length + 1);
    if (!password) {
        return NULL;
    }

    uint8_t random_bytes[128];
    if (!crypto_generate_salt(random_bytes, length)) {
        free(password);
        return NULL;
    }

    for (int i = 0; i < length; i++) {
        password[i] = charset[random_bytes[i] % charset_len];
    }
    password[length] = '\0';

    return password;
}

/* Get database info */
DatabaseInfo* db_transfer_get_info(const char *db_name) {
    if (!db_name || !storage_directory[0]) {
        return NULL;
    }

    DatabaseInfo *info = malloc(sizeof(DatabaseInfo));
    if (!info) {
        return NULL;
    }

    memset(info, 0, sizeof(DatabaseInfo));
    info->name = strdup(db_name);
    info->local_path = make_storage_path(db_name, "db");
    info->encrypted_path = make_storage_path(db_name, "db.enc");

    if (file_exists(info->local_path)) {
        info->size = get_file_size(info->local_path);
        info->modified = get_file_mtime(info->local_path);
        info->is_encrypted = false;
    } else if (file_exists(info->encrypted_path)) {
        info->size = get_file_size(info->encrypted_path);
        info->modified = get_file_mtime(info->encrypted_path);
        info->is_encrypted = true;
    } else {
        db_transfer_free_info(info);
        return NULL;
    }

    return info;
}

void db_transfer_free_info(DatabaseInfo *info) {
    if (!info) return;
    free(info->name);
    free(info->local_path);
    free(info->encrypted_path);
    free(info);
}

/* Verify database integrity */
bool db_transfer_verify_integrity(const char *db_name) {
    if (!db_name || !storage_directory[0]) {
        return false;
    }

    char *local_path = make_storage_path(db_name, "db");
    char *enc_path = make_storage_path(db_name, "db.enc");

    bool result = false;

    if (file_exists(local_path)) {
        // Simple check: can we open it?
        FILE *f = fopen(local_path, "rb");
        if (f) {
            char header[16];
            if (fread(header, 1, 16, f) == 16) {
                // Check SQLite magic
                result = (memcmp(header, "SQLite format 3\0", 16) == 0);
            }
            fclose(f);
        }
    } else if (file_exists(enc_path)) {
        // Check encrypted file header
        FILE *f = fopen(enc_path, "rb");
        if (f) {
            CryptoHeader hdr;
            if (fread(&hdr, sizeof(CryptoHeader), 1, f) == 1) {
                const uint8_t magic[8] = {'C', 'R', 'Y', 'S', 'D', 'B', 'E', 'X'};
                result = (memcmp(hdr.magic, magic, 8) == 0);
            }
            fclose(f);
        }
    }

    free(local_path);
    free(enc_path);

    return result;
}
