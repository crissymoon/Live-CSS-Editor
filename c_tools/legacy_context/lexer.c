#include "lexer.h"
#include <string.h>
#include <ctype.h>

/* Character classification functions */
bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

bool is_alphanumeric(char c) {
    return is_alpha(c) || is_digit(c);
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

bool is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '!' || c == '<' || c == '>' || c == '&' ||
           c == '|' || c == '^' || c == '~' || c == '?' || c == ':';
}

/* Lexer creation and destruction */
Lexer* lexer_create(const char *source) {
    if (!source) return NULL;

    Lexer *lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;

    lexer->source = source;
    lexer->source_length = strlen(source);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current_char = lexer->source_length > 0 ? source[0] : '\0';
    lexer->eof = lexer->source_length == 0;

    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer);
    }
}

/* Lexer navigation */
void lexer_advance(Lexer *lexer) {
    if (lexer->eof) return;

    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    lexer->position++;

    if (lexer->position >= lexer->source_length) {
        lexer->current_char = '\0';
        lexer->eof = true;
    } else {
        lexer->current_char = lexer->source[lexer->position];
    }
}

char lexer_peek(Lexer *lexer, int offset) {
    size_t peek_pos = lexer->position + offset;
    if (peek_pos >= lexer->source_length) {
        return '\0';
    }
    return lexer->source[peek_pos];
}

void lexer_skip_whitespace(Lexer *lexer) {
    while (!lexer->eof && is_whitespace(lexer->current_char)) {
        lexer_advance(lexer);
    }
}

void lexer_skip_comment(Lexer *lexer) {
    /* Single-line comment */
    if (lexer->current_char == '/' && lexer_peek(lexer, 1) == '/') {
        while (!lexer->eof && lexer->current_char != '\n') {
            lexer_advance(lexer);
        }
        return;
    }

    /* Multi-line comment */
    if (lexer->current_char == '/' && lexer_peek(lexer, 1) == '*') {
        lexer_advance(lexer); /* Skip / */
        lexer_advance(lexer); /* Skip * */

        while (!lexer->eof) {
            if (lexer->current_char == '*' && lexer_peek(lexer, 1) == '/') {
                lexer_advance(lexer); /* Skip * */
                lexer_advance(lexer); /* Skip / */
                break;
            }
            lexer_advance(lexer);
        }
        return;
    }

    /* Python/Shell style comment */
    if (lexer->current_char == '#') {
        while (!lexer->eof && lexer->current_char != '\n') {
            lexer_advance(lexer);
        }
        return;
    }
}

/* Token creation and destruction */
Token* token_create(TokenType type, const char *value, int line, int column) {
    Token *token = (Token*)malloc(sizeof(Token));
    if (!token) return NULL;

    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    token->length = value ? strlen(value) : 0;
    token->next = NULL;

    return token;
}

void token_destroy(Token *token) {
    if (token) {
        if (token->value) free(token->value);
        free(token);
    }
}

void lexer_free_tokens(Token *tokens) {
    Token *current = tokens;
    while (current) {
        Token *next = current->next;
        token_destroy(current);
        current = next;
    }
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_KEYWORD: return "KEYWORD";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_STRING: return "STRING";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_COMMENT: return "COMMENT";
        case TOKEN_WHITESPACE: return "WHITESPACE";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_PREPROCESSOR: return "PREPROCESSOR";
        default: return "UNKNOWN";
    }
}

/* Keyword detection */
static bool is_keyword(const char *str) {
    /* C/C++ keywords */
    const char *keywords[] = {
        "auto", "break", "case", "char", "const", "continue", "default", "do",
        "double", "else", "enum", "extern", "float", "for", "goto", "if",
        "int", "long", "register", "return", "short", "signed", "sizeof", "static",
        "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while",
        "class", "namespace", "template", "public", "private", "protected", "virtual",
        /* Python keywords */
        "def", "import", "from", "as", "pass", "lambda", "yield", "assert",
        "del", "except", "finally", "global", "nonlocal", "raise", "try", "with",
        "async", "await", "True", "False", "None", "and", "or", "not", "is", "in",
        /* JavaScript keywords */
        "function", "var", "let", "const", "new", "this", "delete", "typeof",
        "instanceof", "throw", "catch", "finally", "debugger", "export",
        NULL
    };

    for (int i = 0; keywords[i] != NULL; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* Tokenize identifier */
static Token* lexer_read_identifier(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[MAX_TOKEN_LENGTH];
    int i = 0;

    while (!lexer->eof && is_alphanumeric(lexer->current_char) && i < MAX_TOKEN_LENGTH - 1) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    buffer[i] = '\0';

    TokenType type = is_keyword(buffer) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER;
    return token_create(type, buffer, start_line, start_column);
}

/* Tokenize number */
static Token* lexer_read_number(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[MAX_TOKEN_LENGTH];
    int i = 0;
    bool has_dot = false;

    while (!lexer->eof && (is_digit(lexer->current_char) || lexer->current_char == '.') &&
           i < MAX_TOKEN_LENGTH - 1) {
        if (lexer->current_char == '.') {
            if (has_dot) break; /* Second dot, stop */
            has_dot = true;
        }
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }

    /* Handle scientific notation */
    if (!lexer->eof && (lexer->current_char == 'e' || lexer->current_char == 'E')) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
        if (!lexer->eof && (lexer->current_char == '+' || lexer->current_char == '-')) {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        while (!lexer->eof && is_digit(lexer->current_char) && i < MAX_TOKEN_LENGTH - 1) {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
    }

    buffer[i] = '\0';
    return token_create(TOKEN_NUMBER, buffer, start_line, start_column);
}

/* Tokenize string */
static Token* lexer_read_string(Lexer *lexer, char quote) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[MAX_TOKEN_LENGTH];
    int i = 0;

    lexer_advance(lexer); /* Skip opening quote */

    while (!lexer->eof && lexer->current_char != quote && i < MAX_TOKEN_LENGTH - 1) {
        if (lexer->current_char == '\\') {
            lexer_advance(lexer);
            if (!lexer->eof) {
                buffer[i++] = lexer->current_char;
                lexer_advance(lexer);
            }
        } else {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
    }

    if (lexer->current_char == quote) {
        lexer_advance(lexer); /* Skip closing quote */
    }

    buffer[i] = '\0';
    return token_create(TOKEN_STRING, buffer, start_line, start_column);
}

/* Tokenize operator */
static Token* lexer_read_operator(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[MAX_TOKEN_LENGTH];
    int i = 0;

    while (!lexer->eof && is_operator_char(lexer->current_char) && i < MAX_TOKEN_LENGTH - 1) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);

        /* Don't consume multiple single-char operators as one */
        if (i == 1 && !is_operator_char(lexer->current_char)) break;
        if (i == 2) break; /* Max 2-char operators */
    }
    buffer[i] = '\0';

    return token_create(TOKEN_OPERATOR, buffer, start_line, start_column);
}

/* Get next token */
Token* lexer_next_token(Lexer *lexer) {
    if (!lexer || lexer->eof) {
        return token_create(TOKEN_EOF, "", 1, 1);
    }

    /* Skip whitespace and comments */
    while (!lexer->eof) {
        if (is_whitespace(lexer->current_char)) {
            lexer_skip_whitespace(lexer);
        } else if ((lexer->current_char == '/' && (lexer_peek(lexer, 1) == '/' ||
                   lexer_peek(lexer, 1) == '*')) || lexer->current_char == '#') {
            lexer_skip_comment(lexer);
        } else {
            break;
        }
    }

    if (lexer->eof) {
        return token_create(TOKEN_EOF, "", lexer->line, lexer->column);
    }

    /* Newline */
    if (lexer->current_char == '\n') {
        int line = lexer->line;
        int column = lexer->column;
        lexer_advance(lexer);
        return token_create(TOKEN_NEWLINE, "\n", line, column);
    }

    /* Preprocessor directive */
    if (lexer->current_char == '#' && lexer->column == 1) {
        int start_line = lexer->line;
        int start_column = lexer->column;
        char buffer[MAX_TOKEN_LENGTH];
        int i = 0;

        while (!lexer->eof && lexer->current_char != '\n' && i < MAX_TOKEN_LENGTH - 1) {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        buffer[i] = '\0';
        return token_create(TOKEN_PREPROCESSOR, buffer, start_line, start_column);
    }

    /* Identifier or keyword */
    if (is_alpha(lexer->current_char)) {
        return lexer_read_identifier(lexer);
    }

    /* Number */
    if (is_digit(lexer->current_char)) {
        return lexer_read_number(lexer);
    }

    /* String */
    if (lexer->current_char == '"' || lexer->current_char == '\'') {
        char quote = lexer->current_char;
        return lexer_read_string(lexer, quote);
    }

    /* Operator */
    if (is_operator_char(lexer->current_char)) {
        return lexer_read_operator(lexer);
    }

    /* Punctuation */
    char punct = lexer->current_char;
    int line = lexer->line;
    int column = lexer->column;
    lexer_advance(lexer);
    char punct_str[2] = {punct, '\0'};
    return token_create(TOKEN_PUNCTUATION, punct_str, line, column);
}

/* Tokenize entire source */
Token* lexer_tokenize_all(Lexer *lexer, int *token_count) {
    if (!lexer || !token_count) return NULL;

    Token *head = NULL;
    Token *tail = NULL;
    int count = 0;

    while (!lexer->eof) {
        Token *token = lexer_next_token(lexer);
        if (!token) break;

        if (token->type == TOKEN_EOF) {
            token_destroy(token);
            break;
        }

        if (!head) {
            head = token;
            tail = token;
        } else {
            tail->next = token;
            tail = token;
        }
        count++;
    }

    *token_count = count;
    return head;
}
