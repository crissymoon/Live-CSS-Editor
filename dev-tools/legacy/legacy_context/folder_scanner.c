#include "folder_scanner.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>

/* Scanner creation */
FolderScanner* folder_scanner_create(Logger *logger) {
    FolderScanner *scanner = (FolderScanner*)malloc(sizeof(FolderScanner));
    if (!scanner) return NULL;

    scanner->files = NULL;
    scanner->file_count = 0;
    scanner->directory_count = 0;
    scanner->logger = logger;
    scanner->recursive = true;
    scanner->extensions_filter = NULL;
    scanner->extensions_count = 0;

    if (logger) {
        LOG_INFO(logger, "SCANNER", "Folder scanner created");
    }

    return scanner;
}

void folder_scanner_destroy(FolderScanner *scanner) {
    if (!scanner) return;

    folder_scanner_free_files(scanner->files);

    if (scanner->extensions_filter) {
        for (int i = 0; i < scanner->extensions_count; i++) {
            if (scanner->extensions_filter[i]) {
                free(scanner->extensions_filter[i]);
            }
        }
        free(scanner->extensions_filter);
    }

    if (scanner->logger) {
        LOG_INFO(scanner->logger, "SCANNER", "Folder scanner destroyed");
    }

    free(scanner);
}

/* Configuration */
void folder_scanner_set_recursive(FolderScanner *scanner, bool recursive) {
    if (scanner) {
        scanner->recursive = recursive;
    }
}

void folder_scanner_add_extension_filter(FolderScanner *scanner, const char *extension) {
    if (!scanner || !extension) return;

    scanner->extensions_count++;
    scanner->extensions_filter = (char**)realloc(scanner->extensions_filter,
                                                  (scanner->extensions_count + 1) * sizeof(char*));

    scanner->extensions_filter[scanner->extensions_count - 1] = strdup(extension);
    scanner->extensions_filter[scanner->extensions_count] = NULL;

    if (scanner->logger) {
        LOG_DEBUG(scanner->logger, "SCANNER", "Added extension filter: %s", extension);
    }
}

void folder_scanner_clear_filters(FolderScanner *scanner) {
    if (!scanner || !scanner->extensions_filter) return;

    for (int i = 0; i < scanner->extensions_count; i++) {
        if (scanner->extensions_filter[i]) {
            free(scanner->extensions_filter[i]);
        }
    }
    free(scanner->extensions_filter);

    scanner->extensions_filter = NULL;
    scanner->extensions_count = 0;
}

/* Utilities */
const char* folder_scanner_get_extension(const char *path) {
    if (!path) return NULL;

    const char *dot = strrchr(path, '.');
    if (!dot || dot == path) return NULL;

    return dot + 1;
}

bool folder_scanner_is_source_file(const char *path) {
    if (!path) return false;

    const char *ext = folder_scanner_get_extension(path);
    if (!ext) return false;

    /* Common source file extensions */
    const char *source_exts[] = {
        "c", "h", "cpp", "hpp", "cc", "cxx",
        "py", "js", "ts", "jsx", "tsx",
        "java", "swift", "m", "mm",
        "go", "rs", "rb", "php",
        "html", "css", "scss", "sass",
        "json", "xml", "yaml", "yml",
        NULL
    };

    for (int i = 0; source_exts[i] != NULL; i++) {
        if (strcasecmp(ext, source_exts[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Helper to check if extension matches filter */
static bool matches_filter(FolderScanner *scanner, const char *path) {
    if (!scanner->extensions_filter || scanner->extensions_count == 0) {
        return true;  /* No filter means accept all */
    }

    const char *ext = folder_scanner_get_extension(path);
    if (!ext) return false;

    for (int i = 0; i < scanner->extensions_count; i++) {
        if (strcasecmp(ext, scanner->extensions_filter[i]) == 0) {
            return true;
        }
    }

    return false;
}

/* Add file entry */
static void add_file_entry(FolderScanner *scanner, const char *path, const char *name, bool is_dir) {
    if (!scanner || !path) return;

    struct stat st;
    if (stat(path, &st) != 0) return;

    FileEntry *entry = (FileEntry*)malloc(sizeof(FileEntry));
    if (!entry) return;

    entry->path = strdup(path);
    entry->name = name ? strdup(name) : NULL;
    entry->extension = folder_scanner_get_extension(path) ?
                      strdup(folder_scanner_get_extension(path)) : NULL;
    entry->size = st.st_size;
    entry->is_directory = is_dir;
    entry->next = scanner->files;

    scanner->files = entry;

    if (is_dir) {
        scanner->directory_count++;
    } else {
        scanner->file_count++;
    }
}

/* Recursive scan helper */
static void scan_directory_recursive(FolderScanner *scanner, const char *dir_path) {
    if (!scanner || !dir_path) return;

    DIR *dir = opendir(dir_path);
    if (!dir) {
        if (scanner->logger) {
            LOG_WARNING(scanner->logger, "SCANNER", "Cannot open directory: %s", dir_path);
        }
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        /* Skip hidden files */
        if (entry->d_name[0] == '.') {
            continue;
        }

        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) != 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            add_file_entry(scanner, full_path, entry->d_name, true);

            /* Recurse if enabled */
            if (scanner->recursive) {
                scan_directory_recursive(scanner, full_path);
            }
        } else if (S_ISREG(st.st_mode)) {
            /* Check filter */
            if (matches_filter(scanner, full_path)) {
                add_file_entry(scanner, full_path, entry->d_name, false);
            }
        }
    }

    closedir(dir);
}

/* Main scan function */
int folder_scanner_scan(FolderScanner *scanner, const char *root_path) {
    if (!scanner || !root_path) return -1;

    if (scanner->logger) {
        LOG_INFO(scanner->logger, "SCANNER", "Starting scan: %s (recursive: %s)",
                root_path, scanner->recursive ? "yes" : "no");
    }

    /* Clear existing files */
    if (scanner->files) {
        folder_scanner_free_files(scanner->files);
        scanner->files = NULL;
        scanner->file_count = 0;
        scanner->directory_count = 0;
    }

    /* Check if path exists */
    struct stat st;
    if (stat(root_path, &st) != 0) {
        if (scanner->logger) {
            LOG_ERROR(scanner->logger, "SCANNER", "Path does not exist: %s", root_path);
        }
        return -1;
    }

    /* Scan */
    if (S_ISDIR(st.st_mode)) {
        scan_directory_recursive(scanner, root_path);
    } else if (S_ISREG(st.st_mode)) {
        /* Single file */
        if (matches_filter(scanner, root_path)) {
            add_file_entry(scanner, root_path, root_path, false);
        }
    }

    if (scanner->logger) {
        LOG_INFO(scanner->logger, "SCANNER", "Scan complete: %d files, %d directories",
                scanner->file_count, scanner->directory_count);
    }

    return scanner->file_count;
}

/* Get files */
FileEntry* folder_scanner_get_files(FolderScanner *scanner, int *count) {
    if (!scanner || !count) return NULL;

    *count = scanner->file_count;
    return scanner->files;
}

/* Filter by extension */
FileEntry* folder_scanner_filter_by_extension(FolderScanner *scanner, const char *extension, int *count) {
    if (!scanner || !extension || !count) return NULL;

    FileEntry *filtered = NULL;
    int filtered_count = 0;

    FileEntry *current = scanner->files;
    while (current) {
        if (current->extension && strcasecmp(current->extension, extension) == 0) {
            FileEntry *copy = (FileEntry*)malloc(sizeof(FileEntry));
            if (copy) {
                copy->path = strdup(current->path);
                copy->name = current->name ? strdup(current->name) : NULL;
                copy->extension = current->extension ? strdup(current->extension) : NULL;
                copy->size = current->size;
                copy->is_directory = current->is_directory;
                copy->next = filtered;
                filtered = copy;
                filtered_count++;
            }
        }
        current = current->next;
    }

    *count = filtered_count;
    return filtered;
}

/* Filter by size */
FileEntry* folder_scanner_filter_by_size(FolderScanner *scanner, long min_size, long max_size, int *count) {
    if (!scanner || !count) return NULL;

    FileEntry *filtered = NULL;
    int filtered_count = 0;

    FileEntry *current = scanner->files;
    while (current) {
        if (current->size >= min_size && (max_size == 0 || current->size <= max_size)) {
            FileEntry *copy = (FileEntry*)malloc(sizeof(FileEntry));
            if (copy) {
                copy->path = strdup(current->path);
                copy->name = current->name ? strdup(current->name) : NULL;
                copy->extension = current->extension ? strdup(current->extension) : NULL;
                copy->size = current->size;
                copy->is_directory = current->is_directory;
                copy->next = filtered;
                filtered = copy;
                filtered_count++;
            }
        }
        current = current->next;
    }

    *count = filtered_count;
    return filtered;
}

/* Statistics */
void folder_scanner_print_stats(FolderScanner *scanner) {
    if (!scanner || !scanner->logger) return;

    LOG_INFO(scanner->logger, "SCANNER", "===== Scan Statistics =====");
    LOG_INFO(scanner->logger, "SCANNER", "Files: %d", scanner->file_count);
    LOG_INFO(scanner->logger, "SCANNER", "Directories: %d", scanner->directory_count);
    LOG_INFO(scanner->logger, "SCANNER", "Total size: %ld bytes",
            folder_scanner_get_total_size(scanner));
}

long folder_scanner_get_total_size(FolderScanner *scanner) {
    if (!scanner) return 0;

    long total = 0;
    FileEntry *current = scanner->files;

    while (current) {
        if (!current->is_directory) {
            total += current->size;
        }
        current = current->next;
    }

    return total;
}

/* Free files */
void folder_scanner_free_files(FileEntry *files) {
    FileEntry *current = files;
    while (current) {
        FileEntry *next = current->next;
        if (current->path) free(current->path);
        if (current->name) free(current->name);
        if (current->extension) free(current->extension);
        free(current);
        current = next;
    }
}
