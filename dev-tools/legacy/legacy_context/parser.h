#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

/* AST Node types */
typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_VARIABLE,
    AST_PARAMETER,
    AST_BLOCK,
    AST_EXPRESSION,
    AST_STATEMENT,
    AST_IF_STATEMENT,
    AST_LOOP_STATEMENT,
    AST_RETURN_STATEMENT,
    AST_ASSIGNMENT,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_CALL,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_UNKNOWN
} ASTNodeType;

/* AST Node structure */
typedef struct ASTNode {
    ASTNodeType type;
    char *name;
    char *value;
    int line;
    int column;
    struct ASTNode **children;
    int child_count;
    int child_capacity;
    struct ASTNode *parent;
    void *metadata; /* Language-specific data */
} ASTNode;

/* Parser structure */
typedef struct Parser {
    Token *tokens;
    Token *current_token;
    int token_index;
    int token_count;
    ASTNode *root;
    bool has_error;
    char *error_message;
} Parser;

/* Symbol table entry */
typedef struct Symbol {
    char *name;
    char *type;
    int line;
    int column;
    bool is_used;
    struct Symbol *next;
} Symbol;

/* Symbol table structure */
typedef struct SymbolTable {
    Symbol **buckets;
    int size;
    struct SymbolTable *parent; /* For nested scopes */
} SymbolTable;

/* Parser API */
Parser* parser_create(Token *tokens, int token_count);
void parser_destroy(Parser *parser);

ASTNode* parser_parse(Parser *parser);
bool parser_has_error(Parser *parser);
const char* parser_get_error(Parser *parser);

/* AST Node utilities */
ASTNode* ast_node_create(ASTNodeType type, const char *name, int line, int column);
void ast_node_destroy(ASTNode *node);
void ast_node_add_child(ASTNode *parent, ASTNode *child);
const char* ast_node_type_to_string(ASTNodeType type);

/* AST traversal */
typedef void (*ASTVisitor)(ASTNode *node, void *context);
void ast_traverse(ASTNode *node, ASTVisitor visitor, void *context);
void ast_print(ASTNode *node, int indent);

/* Symbol table utilities */
SymbolTable* symbol_table_create(int size, SymbolTable *parent);
void symbol_table_destroy(SymbolTable *table);
void symbol_table_insert(SymbolTable *table, const char *name, const char *type, int line, int column);
Symbol* symbol_table_lookup(SymbolTable *table, const char *name);
void symbol_table_mark_used(SymbolTable *table, const char *name);

#endif /* PARSER_H */
