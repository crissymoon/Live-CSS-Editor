#ifndef MERGER_H
#define MERGER_H

#include "sqlite_db.h"
#include <stdbool.h>

/* Merge context structure */
typedef struct MergerContext {
    LinterDB *db;
    char *source_file;
    char *target_file;
    bool interactive_mode;
    void (*approval_callback)(MergeChange *change, void *user_data);
    void *user_data;
} MergerContext;

/* Merge result structure */
typedef struct MergeResult {
    int total_changes;
    int applied_changes;
    int rejected_changes;
    int failed_changes;
    char **error_messages;
    int error_count;
} MergeResult;

/* Merger API */
MergerContext* merger_create(LinterDB *db);
void merger_destroy(MergerContext *ctx);

/* File merging */
int merger_propose_change(MergerContext *ctx, const char *source_file,
                         const char *target_file, int line_number,
                         const char *old_content, const char *new_content);

MergeResult* merger_apply_changes(MergerContext *ctx, const char *file_path);
MergeResult* merger_apply_with_approval(MergerContext *ctx, const char *file_path,
                                        bool (*approval_func)(MergeChange*, void*),
                                        void *user_data);

/* Interactive approval */
void merger_set_interactive(MergerContext *ctx, bool enabled);
void merger_set_approval_callback(MergerContext *ctx,
                                  void (*callback)(MergeChange*, void*),
                                  void *user_data);

/* Merge preview */
char* merger_preview_changes(MergerContext *ctx, const char *file_path);
int merger_get_pending_count(MergerContext *ctx, const char *file_path);

/* Merge result utilities */
void merger_free_result(MergeResult *result);
void merger_print_result(MergeResult *result);

/* File utilities */
char* merger_read_file(const char *file_path);
int merger_write_file(const char *file_path, const char *content);
char** merger_split_lines(const char *content, int *line_count);
void merger_free_lines(char **lines, int line_count);

/* Diff utilities */
typedef struct DiffLine {
    int line_number;
    char *old_line;
    char *new_line;
    bool is_changed;
    struct DiffLine *next;
} DiffLine;

DiffLine* merger_compute_diff(const char *old_content, const char *new_content);
void merger_free_diff(DiffLine *diff);
void merger_print_diff(DiffLine *diff);

#endif /* MERGER_H */
