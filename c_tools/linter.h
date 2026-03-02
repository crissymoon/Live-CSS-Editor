#ifndef LINTER_H
#define LINTER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Version */
#define LINTER_VERSION "1.0.0"

/* Error codes */
#define LINTER_SUCCESS 0
#define LINTER_ERROR_FILE_NOT_FOUND -1
#define LINTER_ERROR_MEMORY -2
#define LINTER_ERROR_INVALID_PARAM -3
#define LINTER_ERROR_DATABASE -4
#define LINTER_ERROR_PARSE -5

/* Maximum sizes */
#define MAX_FILE_PATH 4096
#define MAX_ERROR_MESSAGE 1024
#define MAX_TOKEN_LENGTH 512
#define MAX_RULE_NAME 128

/* Linter issue structure */
typedef struct LinterIssue {
    char *file_path;
    int line_number;
    int column_number;
    char *rule_name;
    char *message;
    int severity; /* 0=info, 1=warning, 2=error, 3=critical */
    struct LinterIssue *next;
} LinterIssue;

/* Linter context structure */
typedef struct LinterContext {
    char *db_path;
    char *source_file;
    LinterIssue *issues;
    int issue_count;
    bool debug_mode;
    void *lexer;
    void *parser;
    void *db_handle;
} LinterContext;

/* Core API functions */
LinterContext* linter_create(const char *db_path);
void linter_destroy(LinterContext *ctx);

int linter_lint_file(LinterContext *ctx, const char *file_path);
int linter_lint_string(LinterContext *ctx, const char *source_code, const char *language);

LinterIssue* linter_get_issues(LinterContext *ctx, int *count);
void linter_free_issues(LinterIssue *issues);

void linter_set_debug(LinterContext *ctx, bool enabled);
const char* linter_get_error_message(int error_code);

/* Issue reporting */
void linter_report_issue(LinterContext *ctx, const char *file_path,
                         int line, int column, const char *rule_name,
                         const char *message, int severity);

#endif /* LINTER_H */
