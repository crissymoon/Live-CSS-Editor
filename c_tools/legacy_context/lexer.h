#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/* Token types - universal for multiple languages */
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_CHAR,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_COMMENT,
    TOKEN_WHITESPACE,
    TOKEN_NEWLINE,
    TOKEN_PREPROCESSOR,
    TOKEN_UNKNOWN
} TokenType;

/* Token structure */
typedef struct Token {
    TokenType type;
    char *value;
    int line;
    int column;
    int length;
    struct Token *next;
} Token;

/* Lexer structure */
typedef struct Lexer {
    const char *source;
    size_t source_length;
    size_t position;
    int line;
    int column;
    char current_char;
    bool eof;
} Lexer;

/* Lexer API */
Lexer* lexer_create(const char *source);
void lexer_destroy(Lexer *lexer);

Token* lexer_next_token(Lexer *lexer);
Token* lexer_tokenize_all(Lexer *lexer, int *token_count);
void lexer_free_tokens(Token *tokens);

/* Token utilities */
Token* token_create(TokenType type, const char *value, int line, int column);
void token_destroy(Token *token);
const char* token_type_to_string(TokenType type);

/* Lexer internal functions */
void lexer_advance(Lexer *lexer);
char lexer_peek(Lexer *lexer, int offset);
void lexer_skip_whitespace(Lexer *lexer);
void lexer_skip_comment(Lexer *lexer);

bool is_alpha(char c);
bool is_digit(char c);
bool is_alphanumeric(char c);
bool is_whitespace(char c);
bool is_operator_char(char c);

#endif /* LEXER_H */
