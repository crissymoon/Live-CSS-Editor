#ifndef FOLDER_SCANNER_H
#define FOLDER_SCANNER_H

#include <stdbool.h>
#include "logger.h"

/* File entry structure */
typedef struct FileEntry {
    char *path;
    char *name;
    char *extension;
    long size;
    bool is_directory;
    struct FileEntry *next;
} FileEntry;

/* Folder scanner structure */
typedef struct FolderScanner {
    FileEntry *files;
    int file_count;
    int directory_count;
    Logger *logger;
    bool recursive;
    char **extensions_filter;  /* NULL-terminated array */
    int extensions_count;
} FolderScanner;

/* Scanner API */
FolderScanner* folder_scanner_create(Logger *logger);
void folder_scanner_destroy(FolderScanner *scanner);

/* Configuration */
void folder_scanner_set_recursive(FolderScanner *scanner, bool recursive);
void folder_scanner_add_extension_filter(FolderScanner *scanner, const char *extension);
void folder_scanner_clear_filters(FolderScanner *scanner);

/* Scanning */
int folder_scanner_scan(FolderScanner *scanner, const char *root_path);
FileEntry* folder_scanner_get_files(FolderScanner *scanner, int *count);

/* Filtering */
FileEntry* folder_scanner_filter_by_extension(FolderScanner *scanner, const char *extension, int *count);
FileEntry* folder_scanner_filter_by_size(FolderScanner *scanner, long min_size, long max_size, int *count);

/* Statistics */
void folder_scanner_print_stats(FolderScanner *scanner);
long folder_scanner_get_total_size(FolderScanner *scanner);

/* Utilities */
void folder_scanner_free_files(FileEntry *files);
bool folder_scanner_is_source_file(const char *path);
const char* folder_scanner_get_extension(const char *path);

#endif /* FOLDER_SCANNER_H */
