#include "linter.h"
#include "lexer.h"
#include "parser.h"
#include "sqlite_db.h"
#include <regex.h>

/* Linter context management */
LinterContext* linter_create(const char *db_path) {
    if (!db_path) return NULL;

    LinterContext *ctx = (LinterContext*)malloc(sizeof(LinterContext));
    if (!ctx) return NULL;

    ctx->db_path = strdup(db_path);
    ctx->source_file = NULL;
    ctx->issues = NULL;
    ctx->issue_count = 0;
    ctx->debug_mode = false;
    ctx->lexer = NULL;
    ctx->parser = NULL;

    /* Initialize database */
    ctx->db_handle = db_create(db_path);
    if (!ctx->db_handle) {
        free(ctx->db_path);
        free(ctx);
        return NULL;
    }

    LinterDB *db = (LinterDB*)ctx->db_handle;
    if (db_open(db) != 0) {
        db_destroy(db);
        free(ctx->db_path);
        free(ctx);
        return NULL;
    }

    /* Initialize schema */
    db_initialize_schema(db);

    return ctx;
}

void linter_destroy(LinterContext *ctx) {
    if (!ctx) return;

    if (ctx->db_path) free(ctx->db_path);
    if (ctx->source_file) free(ctx->source_file);
    if (ctx->issues) linter_free_issues(ctx->issues);

    if (ctx->db_handle) {
        LinterDB *db = (LinterDB*)ctx->db_handle;
        db_close(db);
        db_destroy(db);
    }

    free(ctx);
}

void linter_set_debug(LinterContext *ctx, bool enabled) {
    if (ctx) {
        ctx->debug_mode = enabled;
    }
}

const char* linter_get_error_message(int error_code) {
    switch (error_code) {
        case LINTER_SUCCESS: return "Success";
        case LINTER_ERROR_FILE_NOT_FOUND: return "File not found";
        case LINTER_ERROR_MEMORY: return "Memory allocation error";
        case LINTER_ERROR_INVALID_PARAM: return "Invalid parameter";
        case LINTER_ERROR_DATABASE: return "Database error";
        case LINTER_ERROR_PARSE: return "Parse error";
        default: return "Unknown error";
    }
}

/* Issue management */
void linter_report_issue(LinterContext *ctx, const char *file_path,
                         int line, int column, const char *rule_name,
                         const char *message, int severity) {
    if (!ctx) return;

    LinterIssue *issue = (LinterIssue*)malloc(sizeof(LinterIssue));
    if (!issue) return;

    issue->file_path = file_path ? strdup(file_path) : NULL;
    issue->line_number = line;
    issue->column_number = column;
    issue->rule_name = rule_name ? strdup(rule_name) : NULL;
    issue->message = message ? strdup(message) : NULL;
    issue->severity = severity;
    issue->next = NULL;

    /* Add to linked list */
    if (!ctx->issues) {
        ctx->issues = issue;
    } else {
        LinterIssue *current = ctx->issues;
        while (current->next) {
            current = current->next;
        }
        current->next = issue;
    }

    ctx->issue_count++;

    /* Track in database */
    if (ctx->db_handle && file_path && rule_name) {
        LinterDB *db = (LinterDB*)ctx->db_handle;
        db_track_issue(db, file_path, rule_name, line, NULL);
    }

    if (ctx->debug_mode) {
        printf("[%s] %s:%d:%d - %s\n",
               rule_name ? rule_name : "UNKNOWN",
               file_path ? file_path : "?",
               line, column,
               message ? message : "No message");
    }
}

LinterIssue* linter_get_issues(LinterContext *ctx, int *count) {
    if (!ctx || !count) return NULL;

    *count = ctx->issue_count;
    return ctx->issues;
}

void linter_free_issues(LinterIssue *issues) {
    LinterIssue *current = issues;
    while (current) {
        LinterIssue *next = current->next;
        if (current->file_path) free(current->file_path);
        if (current->rule_name) free(current->rule_name);
        if (current->message) free(current->message);
        free(current);
        current = next;
    }
}

/* Pattern matching against AST */
static void check_patterns_visitor(ASTNode *node, void *context) {
    LinterContext *ctx = (LinterContext*)context;
    if (!ctx || !ctx->db_handle) return;

    LinterDB *db = (LinterDB*)ctx->db_handle;

    /* Get patterns from database */
    int pattern_count = 0;
    Pattern *patterns = db_get_patterns(db, NULL, &pattern_count);

    if (!patterns) return;

    /* Check each pattern */
    Pattern *current_pattern = patterns;
    while (current_pattern) {
        if (!current_pattern->enabled) {
            current_pattern = current_pattern->next;
            continue;
        }

        /* Compile regex */
        regex_t regex;
        if (regcomp(&regex, current_pattern->regex_pattern, REG_EXTENDED | REG_NOSUB) != 0) {
            current_pattern = current_pattern->next;
            continue;
        }

        /* Check against node name and value */
        bool matched = false;

        if (node->name && regexec(&regex, node->name, 0, NULL, 0) == 0) {
            matched = true;
        } else if (node->value && regexec(&regex, node->value, 0, NULL, 0) == 0) {
            matched = true;
        }

        if (matched) {
            char message[MAX_ERROR_MESSAGE];
            snprintf(message, sizeof(message), "%s: %s",
                    current_pattern->name,
                    current_pattern->description ? current_pattern->description : "Pattern matched");

            linter_report_issue(ctx, ctx->source_file, node->line, node->column,
                              current_pattern->name, message, current_pattern->severity);
        }

        regfree(&regex);
        current_pattern = current_pattern->next;
    }

    db_free_patterns(patterns);
}

/* Semantic checks */
static void check_unused_variables(ASTNode *root, LinterContext *ctx) {
    if (!root || !ctx) return;

    /* Create symbol table */
    SymbolTable *symbols = symbol_table_create(128, NULL);
    if (!symbols) return;

    /* First pass: collect all variable declarations */
    for (int i = 0; i < root->child_count; i++) {
        ASTNode *node = root->children[i];
        if (node->type == AST_VARIABLE && node->child_count > 0) {
            ASTNode *id = node->children[0];
            if (id->type == AST_IDENTIFIER && id->name) {
                symbol_table_insert(symbols, id->name, "variable", id->line, id->column);
            }
        }
    }

    /* Second pass: mark used variables */
    for (int i = 0; i < root->child_count; i++) {
        ASTNode *node = root->children[i];
        for (int j = 0; j < node->child_count; j++) {
            ASTNode *child = node->children[j];
            if (child->type == AST_IDENTIFIER && child->name) {
                symbol_table_mark_used(symbols, child->name);
            }
        }
    }

    /* Third pass: report unused variables */
    for (int i = 0; i < symbols->size; i++) {
        Symbol *sym = symbols->buckets[i];
        while (sym) {
            if (!sym->is_used) {
                char message[MAX_ERROR_MESSAGE];
                snprintf(message, sizeof(message), "Unused variable: %s", sym->name);
                linter_report_issue(ctx, ctx->source_file, sym->line, sym->column,
                                  "unused-variable", message, 1);
            }
            sym = sym->next;
        }
    }

    symbol_table_destroy(symbols);
}

/* Main linting functions */
int linter_lint_string(LinterContext *ctx, const char *source_code, const char *language) {
    if (!ctx || !source_code) return LINTER_ERROR_INVALID_PARAM;

    /* Clear previous issues */
    if (ctx->issues) {
        linter_free_issues(ctx->issues);
        ctx->issues = NULL;
        ctx->issue_count = 0;
    }

    /* Lexical analysis */
    Lexer *lexer = lexer_create(source_code);
    if (!lexer) return LINTER_ERROR_MEMORY;

    ctx->lexer = lexer;

    int token_count = 0;
    Token *tokens = lexer_tokenize_all(lexer, &token_count);

    if (!tokens || token_count == 0) {
        lexer_destroy(lexer);
        ctx->lexer = NULL;
        return LINTER_ERROR_PARSE;
    }

    if (ctx->debug_mode) {
        printf("[DEBUG] Tokenized %d tokens\n", token_count);
    }

    /* Syntactic analysis */
    Parser *parser = parser_create(tokens, token_count);
    if (!parser) {
        lexer_free_tokens(tokens);
        lexer_destroy(lexer);
        ctx->lexer = NULL;
        return LINTER_ERROR_MEMORY;
    }

    ctx->parser = parser;

    ASTNode *root = parser_parse(parser);

    if (parser_has_error(parser)) {
        const char *error = parser_get_error(parser);
        linter_report_issue(ctx, ctx->source_file, 1, 1, "parse-error",
                          error ? error : "Parse error", 2);
    }

    if (ctx->debug_mode && root) {
        printf("[DEBUG] AST:\n");
        ast_print(root, 0);
    }

    /* Pattern-based checks */
    if (root) {
        ast_traverse(root, check_patterns_visitor, ctx);
        check_unused_variables(root, ctx);
    }

    /* Cleanup */
    parser_destroy(parser);
    ctx->parser = NULL;
    lexer_free_tokens(tokens);
    lexer_destroy(lexer);
    ctx->lexer = NULL;

    return LINTER_SUCCESS;
}

int linter_lint_file(LinterContext *ctx, const char *file_path) {
    if (!ctx || !file_path) return LINTER_ERROR_INVALID_PARAM;

    /* Read file */
    FILE *file = fopen(file_path, "r");
    if (!file) return LINTER_ERROR_FILE_NOT_FOUND;

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = (char*)malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return LINTER_ERROR_MEMORY;
    }

    size_t read_size = fread(content, 1, file_size, file);
    content[read_size] = '\0';
    fclose(file);

    /* Set source file */
    if (ctx->source_file) free(ctx->source_file);
    ctx->source_file = strdup(file_path);

    /* Detect language from extension */
    const char *ext = strrchr(file_path, '.');
    const char *language = ext ? ext + 1 : "unknown";

    if (ctx->debug_mode) {
        printf("[DEBUG] Linting file: %s (language: %s)\n", file_path, language);
    }

    /* Lint */
    int result = linter_lint_string(ctx, content, language);

    free(content);
    return result;
}
