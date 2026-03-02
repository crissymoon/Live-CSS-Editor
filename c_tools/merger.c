#include "merger.h"
#include <string.h>
#include <stdio.h>

/* Merger creation and destruction */
MergerContext* merger_create(LinterDB *db) {
    if (!db) return NULL;

    MergerContext *ctx = (MergerContext*)malloc(sizeof(MergerContext));
    if (!ctx) return NULL;

    ctx->db = db;
    ctx->source_file = NULL;
    ctx->target_file = NULL;
    ctx->interactive_mode = false;
    ctx->approval_callback = NULL;
    ctx->user_data = NULL;

    return ctx;
}

void merger_destroy(MergerContext *ctx) {
    if (!ctx) return;

    if (ctx->source_file) free(ctx->source_file);
    if (ctx->target_file) free(ctx->target_file);
    free(ctx);
}

/* File utilities */
char* merger_read_file(const char *file_path) {
    if (!file_path) return NULL;

    FILE *file = fopen(file_path, "r");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }

    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';

    fclose(file);
    return content;
}

int merger_write_file(const char *file_path, const char *content) {
    if (!file_path || !content) return -1;

    FILE *file = fopen(file_path, "w");
    if (!file) return -1;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, file);

    fclose(file);
    return (written == len) ? 0 : -1;
}

char** merger_split_lines(const char *content, int *line_count) {
    if (!content || !line_count) return NULL;

    /* Count lines */
    int count = 1;
    for (const char *p = content; *p; p++) {
        if (*p == '\n') count++;
    }

    char **lines = (char**)malloc(count * sizeof(char*));
    if (!lines) return NULL;

    int line_index = 0;
    const char *line_start = content;

    for (const char *p = content; *p; p++) {
        if (*p == '\n' || *(p + 1) == '\0') {
            int line_length = p - line_start + (*(p + 1) == '\0' ? 1 : 0);
            lines[line_index] = (char*)malloc(line_length + 1);
            strncpy(lines[line_index], line_start, line_length);
            lines[line_index][line_length] = '\0';

            line_index++;
            line_start = p + 1;
        }
    }

    *line_count = line_index;
    return lines;
}

void merger_free_lines(char **lines, int line_count) {
    if (!lines) return;

    for (int i = 0; i < line_count; i++) {
        if (lines[i]) free(lines[i]);
    }
    free(lines);
}

/* Merge operations */
int merger_propose_change(MergerContext *ctx, const char *source_file,
                         const char *target_file, int line_number,
                         const char *old_content, const char *new_content) {
    if (!ctx || !ctx->db || !source_file || !target_file) return -1;

    return db_add_merge_change(ctx->db, source_file, target_file, line_number,
                               old_content, new_content);
}

int merger_get_pending_count(MergerContext *ctx, const char *file_path) {
    if (!ctx || !ctx->db || !file_path) return 0;

    int count = 0;
    MergeChange *changes = db_get_pending_changes(ctx->db, file_path, &count);
    db_free_merge_changes(changes);

    return count;
}

void merger_set_interactive(MergerContext *ctx, bool enabled) {
    if (ctx) {
        ctx->interactive_mode = enabled;
    }
}

void merger_set_approval_callback(MergerContext *ctx,
                                  void (*callback)(MergeChange*, void*),
                                  void *user_data) {
    if (ctx) {
        ctx->approval_callback = callback;
        ctx->user_data = user_data;
    }
}

/* Apply changes */
MergeResult* merger_apply_changes(MergerContext *ctx, const char *file_path) {
    if (!ctx || !ctx->db || !file_path) return NULL;

    MergeResult *result = (MergeResult*)malloc(sizeof(MergeResult));
    if (!result) return NULL;

    result->total_changes = 0;
    result->applied_changes = 0;
    result->rejected_changes = 0;
    result->failed_changes = 0;
    result->error_messages = NULL;
    result->error_count = 0;

    /* Get approved changes */
    MergeChange *changes = NULL;
    int change_count = 0;

    if (db_get_approved_changes(ctx->db, file_path, &changes, &change_count) != 0) {
        return result;
    }

    if (change_count == 0) {
        db_free_merge_changes(changes);
        return result;
    }

    result->total_changes = change_count;

    /* Read target file */
    char *content = merger_read_file(file_path);
    if (!content) {
        result->failed_changes = change_count;
        db_free_merge_changes(changes);
        return result;
    }

    /* Split into lines */
    int line_count = 0;
    char **lines = merger_split_lines(content, &line_count);
    free(content);

    if (!lines) {
        result->failed_changes = change_count;
        db_free_merge_changes(changes);
        return result;
    }

    /* Apply each change */
    MergeChange *current = changes;
    while (current) {
        if (current->line_number > 0 && current->line_number <= line_count) {
            /* Replace line */
            if (lines[current->line_number - 1]) {
                free(lines[current->line_number - 1]);
            }
            lines[current->line_number - 1] = strdup(current->new_content);
            result->applied_changes++;
        } else {
            result->failed_changes++;
        }
        current = current->next;
    }

    /* Write back */
    FILE *file = fopen(file_path, "w");
    if (file) {
        for (int i = 0; i < line_count; i++) {
            fprintf(file, "%s\n", lines[i]);
        }
        fclose(file);
    } else {
        result->failed_changes += result->applied_changes;
        result->applied_changes = 0;
    }

    merger_free_lines(lines, line_count);
    db_free_merge_changes(changes);

    return result;
}

MergeResult* merger_apply_with_approval(MergerContext *ctx, const char *file_path,
                                        bool (*approval_func)(MergeChange*, void*),
                                        void *user_data) {
    if (!ctx || !ctx->db || !file_path) return NULL;

    MergeResult *result = (MergeResult*)malloc(sizeof(MergeResult));
    if (!result) return NULL;

    result->total_changes = 0;
    result->applied_changes = 0;
    result->rejected_changes = 0;
    result->failed_changes = 0;
    result->error_messages = NULL;
    result->error_count = 0;

    /* Get pending changes */
    int change_count = 0;
    MergeChange *changes = db_get_pending_changes(ctx->db, file_path, &change_count);

    if (!changes || change_count == 0) {
        db_free_merge_changes(changes);
        return result;
    }

    result->total_changes = change_count;

    /* Process each change with approval */
    MergeChange *current = changes;
    while (current) {
        bool approved = false;

        if (approval_func) {
            approved = approval_func(current, user_data);
        } else if (ctx->approval_callback) {
            ctx->approval_callback(current, ctx->user_data);
            approved = true; /* Callback handles approval */
        }

        if (approved) {
            db_approve_change(ctx->db, current->id, true);
            result->applied_changes++;
        } else {
            db_approve_change(ctx->db, current->id, false);
            result->rejected_changes++;
        }

        current = current->next;
    }

    db_free_merge_changes(changes);

    /* Now apply approved changes */
    if (result->applied_changes > 0) {
        MergeResult *apply_result = merger_apply_changes(ctx, file_path);
        if (apply_result) {
            result->applied_changes = apply_result->applied_changes;
            result->failed_changes = apply_result->failed_changes;
            merger_free_result(apply_result);
        }
    }

    return result;
}

/* Preview changes */
char* merger_preview_changes(MergerContext *ctx, const char *file_path) {
    if (!ctx || !ctx->db || !file_path) return NULL;

    int change_count = 0;
    MergeChange *changes = db_get_pending_changes(ctx->db, file_path, &change_count);

    if (!changes || change_count == 0) {
        db_free_merge_changes(changes);
        return strdup("No pending changes");
    }

    /* Build preview string */
    char *preview = (char*)malloc(4096 * change_count);
    if (!preview) {
        db_free_merge_changes(changes);
        return NULL;
    }

    preview[0] = '\0';
    char buffer[4096];

    sprintf(buffer, "Pending changes for %s:\n\n", file_path);
    strcat(preview, buffer);

    MergeChange *current = changes;
    int index = 1;

    while (current) {
        sprintf(buffer, "[Change %d] Line %d:\n", index++, current->line_number);
        strcat(preview, buffer);

        sprintf(buffer, "  OLD: %s\n", current->old_content);
        strcat(preview, buffer);

        sprintf(buffer, "  NEW: %s\n", current->new_content);
        strcat(preview, buffer);

        sprintf(buffer, "  Source: %s\n\n", current->source_file);
        strcat(preview, buffer);

        current = current->next;
    }

    db_free_merge_changes(changes);
    return preview;
}

/* Diff utilities */
DiffLine* merger_compute_diff(const char *old_content, const char *new_content) {
    if (!old_content || !new_content) return NULL;

    int old_line_count = 0;
    int new_line_count = 0;

    char **old_lines = merger_split_lines(old_content, &old_line_count);
    char **new_lines = merger_split_lines(new_content, &new_line_count);

    if (!old_lines || !new_lines) {
        merger_free_lines(old_lines, old_line_count);
        merger_free_lines(new_lines, new_line_count);
        return NULL;
    }

    DiffLine *head = NULL;
    DiffLine *tail = NULL;

    int max_lines = (old_line_count > new_line_count) ? old_line_count : new_line_count;

    for (int i = 0; i < max_lines; i++) {
        DiffLine *diff = (DiffLine*)malloc(sizeof(DiffLine));
        if (!diff) continue;

        diff->line_number = i + 1;
        diff->old_line = (i < old_line_count) ? strdup(old_lines[i]) : NULL;
        diff->new_line = (i < new_line_count) ? strdup(new_lines[i]) : NULL;

        diff->is_changed = false;
        if (diff->old_line && diff->new_line) {
            diff->is_changed = (strcmp(diff->old_line, diff->new_line) != 0);
        } else {
            diff->is_changed = true;
        }

        diff->next = NULL;

        if (!head) {
            head = diff;
            tail = diff;
        } else {
            tail->next = diff;
            tail = diff;
        }
    }

    merger_free_lines(old_lines, old_line_count);
    merger_free_lines(new_lines, new_line_count);

    return head;
}

void merger_free_diff(DiffLine *diff) {
    DiffLine *current = diff;
    while (current) {
        DiffLine *next = current->next;
        if (current->old_line) free(current->old_line);
        if (current->new_line) free(current->new_line);
        free(current);
        current = next;
    }
}

void merger_print_diff(DiffLine *diff) {
    if (!diff) return;

    DiffLine *current = diff;
    while (current) {
        if (current->is_changed) {
            printf("Line %d:\n", current->line_number);
            if (current->old_line) {
                printf("  - %s\n", current->old_line);
            }
            if (current->new_line) {
                printf("  + %s\n", current->new_line);
            }
        }
        current = current->next;
    }
}

/* Result utilities */
void merger_free_result(MergeResult *result) {
    if (!result) return;

    if (result->error_messages) {
        for (int i = 0; i < result->error_count; i++) {
            if (result->error_messages[i]) {
                free(result->error_messages[i]);
            }
        }
        free(result->error_messages);
    }

    free(result);
}

void merger_print_result(MergeResult *result) {
    if (!result) return;

    printf("\n[Merge Result]\n");
    printf("Total changes:    %d\n", result->total_changes);
    printf("Applied:          %d\n", result->applied_changes);
    printf("Rejected:         %d\n", result->rejected_changes);
    printf("Failed:           %d\n", result->failed_changes);

    if (result->error_count > 0) {
        printf("\n[Errors]\n");
        for (int i = 0; i < result->error_count; i++) {
            printf("  - %s\n", result->error_messages[i]);
        }
    }
}
