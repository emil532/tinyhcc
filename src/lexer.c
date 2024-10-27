/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <inttypes.h>
#include <assert.h>

#include "lexer.h"

typedef struct {
    const char* sequence;
    char value;
} EscapeSequence;

static const EscapeSequence escape_sequences[] = {
    {"\\n", '\n'},
    {"\\t", '\t'},
    {"\\r", '\r'},
    {"\\v", '\v'},
    {"\\b", '\b'},
    {"\\f", '\f'},
    {"\\a", '\a'},
    {"\\\\", '\\'},
    {"\\\"", '"'},
    {"\\\'", '\''},
    {NULL, 0} 
};

static const char* keywords[] = {
    "if", "else", "while", "for", "switch", "case", "asm", "try", "catch", "throw", "break", "goto", "class", "union",
    /* Pseudo */
    "no_warn", "reg", "noreg", "static", "extern", /* "import", */
    NULL
};

void freeTokens(Token* tokens, size_t nTokens) {
    if (tokens == NULL) {
        return;
    }

    for (size_t i = 0; i < nTokens; i++) {
        free(tokens[i].value);
        tokens[i].value = NULL;
    }
    free(tokens);
}


static bool appendToken(Token** tokens, size_t* sTokens, size_t* nTokens, const char* file, size_t line, size_t col, Token token) {
    assert(tokens != NULL);
    assert(sTokens != NULL);
    assert(nTokens != NULL);
    if (*nTokens >= *sTokens) {
        size_t newSize = (*sTokens == 0) ? 128 : (*sTokens * 2);
        Token* newTokens = realloc(*tokens, newSize * sizeof(Token));

        if (newTokens == NULL) {
            fprintf(stderr, "%s:%zu:%zu: Memory alloation failed in appendToken\n", file, line, col);
            perror("realloc");
            freeTokens(*tokens, *nTokens);
            *tokens = NULL;
            return false;
        }
        *tokens = newTokens;
        *sTokens = newSize;
    }

    (*tokens)[(*nTokens)++] = token;
    return true;
}

static char* handleEscapeSequence(const char* source, size_t* i, size_t* col, size_t* line, const char* file) {
    (*i)++;
    (*col)++;

    if (!source[*i]) {
        fprintf(stderr, "%s:%zu:%zu: Unterminated escape sequence\n", file, *line, *col);
        return NULL;
    }

    for (const EscapeSequence* es = escape_sequences; es->sequence; ++es) {
        size_t len = strlen(es->sequence);
        if (strncmp(source + *i - 1, es->sequence, len) == 0) {
            *i += len - 1;
            *col += len - 1;
            char* result = malloc(2); 
            if (!result) {
                perror("malloc");
                return NULL;
            }
            result[0] = es->value;
            result[1] = '\0';
            return result;
        }
    }

    if (source[*i] == 'x') {
        (*i)++;
        (*col)++;
        char hex_buffer[9] = { 0 }; 
        int hex_digits = 0;

        while (isxdigit(source[*i]) && hex_digits < 8) {
            hex_buffer[hex_digits++] = source[*i];
            (*i)++;
            (*col)++;
        }

        if (!hex_digits) {
            fprintf(stderr, "%s:%zu:%zu: Expected hexadecimal digits after '\\x'.\n", file, *line, *col);
            return NULL;
        }

        unsigned long long val = strtoull(hex_buffer, NULL, 16);
        if (val > UCHAR_MAX) {
            fprintf(stderr, "%s:%zu:%zu: Hexadecimal escape sequence out of range.\n", file, *line, *col);
        }

        char* result = malloc(2);
        if (!result) {
            perror("malloc");
            return NULL;
        }
        result[0] = (char)val;
        result[1] = '\0';
        return result;
    }
    else if (isdigit(source[*i])) {
        char octal_buffer[4] = { 0 };
        int octal_digits = 0;
        while (source[*i] >= '0' && source[*i] <= '7' && octal_digits < 3) {
            octal_buffer[octal_digits++] = source[*i];
            (*i)++;
            (*col)++;
        }

        if (!octal_digits) {
            fprintf(stderr, "%s:%zu:%zu: Expected octal digits after '\\'.\n", file, *line, *col);
            return NULL;
        }

        unsigned long long val = strtoull(octal_buffer, NULL, 8);
        if (val > UCHAR_MAX) {
            fprintf(stderr, "%s:%zu:%zu: Octal escape sequence out of range.\n", file, *line, *col);
        }

        char* result = malloc(2);
        if (!result) {
            perror("malloc");
            return NULL;
        }
        result[0] = (char)val;
        result[1] = '\0';
        return result;
    }
    else {
        char unrecognized = source[*i];
        (*i)++;
        (*col)++;
        fprintf(stderr, "%s:%zu:%zu: Warning: Unrecognized escape sequence '\\%c'\n", file, *line, *col - 1, unrecognized);

        char* result = malloc(2);
        if (!result) {
            perror("malloc");
            return NULL;
        }
        result[0] = unrecognized;
        result[1] = '\0';
        return result;
    }
}


Token* tokenize(const char* source, const char* file) {
    if (source == NULL || file == NULL) {
        fprintf(stderr, "Error: NULL source or file argument passed to tokenize.\n");
        return NULL;
    }

    Token* tokens = malloc(128 * sizeof(Token)); /* For performance reasons we assume files will have 128 or more tokens */
    size_t sTokens = 128;
    size_t nTokens = 0;
    size_t i = 0;
    size_t line = 1;
    size_t col = 1;

    while (source[i]) {
        switch (source[i]) {
        case '\t':
        case '\r':
        case ' ':
            i++;
            col++;
            break;

        case '\n':
            i++;
            line++;
            col = 1;  
            break;


        case '+': {
            TokenType type = TT_ADD;
            size_t len = 1;

            if (source[i + 1] == '+') {
                type = TT_INC;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '=') {
                type = TT_ADDEQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }
        case '-': {
            TokenType type = TT_SUB;
            size_t len = 1;

            if (source[i + 1] == '-') {
                type = TT_DEC;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '=') {
                type = TT_SUBEQ;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '>') {
                type = TT_ARROW;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '*': {
            TokenType type = TT_MUL;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_MULEQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '/': {
            if (source[i + 1] == '=') {
                Token token = {
                    .type = TT_DIVEQ,
                    .value = NULL,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 2
                };

                if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                    return NULL;
                }

                i += 2;
                col += 2;
            }
            else if (source[i + 1] == '/') { 
                while (source[i] && source[i] != '\n') {
                    i++;
                }
            }
            else if (source[i + 1] == '*') { 
                i++; 
                while (source[++i] && !(source[i] == '*' && source[i + 1] == '/'));

                if (!source[i]) {
                    fprintf(stderr, "%s:%zu:%zu: Reached EOF while parsng block comment.\n", file, line, col);
                    freeTokens(tokens, nTokens);
                    return NULL;
                }
                i++;

            }
            else { 
                Token token = {
                    .type = TT_DIV,
                    .value = NULL,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                };

                if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                    return NULL;
                }

                i++;
                col++;
            }
            break;
        }

        case '%': {
            TokenType type = TT_MOD;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_MODEQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }
                
        case '<': {
            TokenType type = TT_LT;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_LTE;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '<') {
                len = 2;
                i++;
                col++;
                if (source[i + 1] == '=') {
                    type = TT_LSHEQ;
                    len = 3;
                    i++;
                    col++;
                }
                else {
                    type = TT_LSH;
                }
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

                
        case '>': {
            TokenType type = TT_GT;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_GTE;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '>') {
                len = 2;
                i++;
                col++;
                if (source[i + 1] == '=') {
                    type = TT_RSHEQ;
                    len = 3;
                    i++;
                    col++;
                }
                else {
                    type = TT_RSH;
                }
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }


        case '~': {
            Token token = {
                .type = TT_BNOT,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '^': {
            TokenType type = TT_BXOR;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_XOREQ;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '^') {  

                type = TT_XOR;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }


        case '`': {
            Token token = {
                .type = TT_POW,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '&': {
            TokenType type = TT_BAND;
            size_t len = 1;

            if (source[i + 1] == '&') { 
                type = TT_AND;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '=') {
                type = TT_ANDEQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '|': {
            TokenType type = TT_BOR;
            size_t len = 1;

            if (source[i + 1] == '|') { 
                type = TT_OR;
                len = 2;
                i++;
                col++;
            }
            else if (source[i + 1] == '=') {
                type = TT_OREQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '=': {
            TokenType type = TT_ASSIGN;
            size_t len = 1;

            if (source[i + 1] == '=') {

                type = TT_EQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }

        case '!': {
            TokenType type = TT_NOT;
            size_t len = 1;

            if (source[i + 1] == '=') {
                type = TT_NEQ;
                len = 2;
                i++;
                col++;
            }

            Token token = {
                .type = type,
                .value = NULL,
                .index = i - len + 1,
                .col = col - len + 1,
                .line = line,
                .len = len
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }

            i++;
            col++;
            break;
        }
        case '(': {
            Token token = {
                .type = TT_LPAREN,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                return NULL;
            }
            i++;
            col++;
            break;
        }

        case ')': {
            Token token = {
                .type = TT_RPAREN,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '{': {
            Token token = {
                .type = TT_LBRACE,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '}': {
            Token token = {
                .type = TT_RBRACE,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '[': {
            Token token = {
                .type = TT_LBRACKET,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case ']': {
            Token token = {
                .type = TT_RBRACKET,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }
        case ';': {
            Token token = {
                .type = TT_SEMICOLON,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }
        case ':': {
            Token token = {
                .type = TT_COLON,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '.': {
            if (isdigit(source[i + 1])) { 
                goto parse_number;
            }
            else if (source[i + 1] == '.' && source[i + 2] == '.') { 
                Token token = {
                    .type = TT_ELLIPSIS,
                    .value = NULL,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 3
                };
                if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
                i += 3;
                col += 3;
            }
            else { 
                Token token = {
                    .type = TT_DOT,
                    .value = NULL,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                };
                if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
                i++;
                col++;
            }
            break;
        }

        case ',': {
            Token token = {
                .type = TT_COMMA,
                .value = NULL,
                .index = i,
                .col = col,
                .line = line,
                .len = 1
            };
            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) return NULL;
            i++;
            col++;
            break;
        }

        case '\'': {
            size_t start = i;
            size_t start_col = col;
            i++;
            col++;

            char* char_value = NULL;

            if (source[i] == '\\') { 
                char_value = handleEscapeSequence(source, &i, &col, &line, file);
                if (!char_value) {
                    freeTokens(tokens, nTokens);
                    return NULL;
                }
            }
            else if (source[i] != '\'') { 
                char_value = malloc(2);
                if (!char_value) {
                    perror("malloc");
                    freeTokens(tokens, nTokens);
                    return NULL;
                }
                char_value[0] = source[i];
                char_value[1] = '\0';
                i++;
                col++;
            }
            else { 
                fprintf(stderr, "%s:%zu:%zu: Empty character constnt.\n", file, line, col);
                freeTokens(tokens, nTokens);
                return NULL;
            }



            if (source[i] != '\'') {
                fprintf(stderr, "%s:%zu:%zu: Unterminated character constant.\n", file, line, col);
                free(char_value);
                freeTokens(tokens, nTokens);
                return NULL;
            }
            i++;
            col++;

            Token token = {
                .type = TT_CHAR,
                .value = char_value,
                .index = start,
                .col = start_col,
                .line = line,
                .len = i - start
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                free(char_value);
                freeTokens(tokens, nTokens);
                return NULL;
            }
            break;
        }
        case '"': {
            size_t start = i;
            size_t start_col = col;
            i++; 
            col++;

            size_t string_length = 0;
            char* string_value = NULL;

            while (source[i] && source[i] != '"') {
                char* escaped = NULL;
                if (source[i] == '\\') { 
                    escaped = handleEscapeSequence(source, &i, &col, &line, file);
                    if (!escaped) { 
                        free(string_value);
                        freeTokens(tokens, nTokens);
                        return NULL;
                    }
                    size_t escaped_len = strlen(escaped);

                    char* new_string_value = realloc(string_value, string_length + escaped_len + 1);
                    if (!new_string_value) {
                        perror("realloc");
                        free(string_value);
                        free(escaped);
                        freeTokens(tokens, nTokens);
                        return NULL;
                    }
                    string_value = new_string_value;
                    memcpy(string_value + string_length, escaped, escaped_len);
                    string_length += escaped_len;
                    free(escaped);

                }
                else { 
                    char* new_string_value = realloc(string_value, string_length + 2); 
                    if (!new_string_value) {
                        perror("realloc");
                        free(string_value);
                        freeTokens(tokens, nTokens);
                        return NULL;
                    }
                    string_value = new_string_value;
                    string_value[string_length++] = source[i];
                    i++;
                    col++;
                }
            }

            if (!source[i]) {
                fprintf(stderr, "%s:%zu:%zu: Unterminated string literal.\n", file, line, col);
                free(string_value);
                freeTokens(tokens, nTokens);
                return NULL;
            }

            i++; 
            col++;

            if (string_value) {
                string_value[string_length] = '\0';
            }
            else {
                string_value = calloc(1, 1);
            }

            Token token = {
                .type = TT_STRING,
                .value = string_value,
                .index = start,
                .col = start_col,
                .line = line,
                .len = i - start
            };

            if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                free(string_value);
                freeTokens(tokens, nTokens);
                return NULL;
            }

            break;
        }
        default: {
            if (isalpha(source[i]) || source[i] == '_') {
                size_t start = i;
                while (isalnum(source[i]) || source[i] == '_') {
                    i++;
                    col++;
                }
                size_t len = i - start;

                char* value = malloc(len + 1); 
                if (value == NULL) {
                    fprintf(stderr, "%s:%zu:%zu: Memory allocation failed.\n", file, line, col);
                    freeTokens(tokens, nTokens);
                    return NULL;
                }
                memcpy(value, source + start, len);
                value[len] = '\0';

                bool isKeyword = false;
                for (const char** kw = keywords; *kw != NULL; kw++) {
                    if (strcmp(value, *kw) == 0) {
                        isKeyword = true;
                        break;
                    }
                }

                Token token = {
                    .type = isKeyword ? TT_KEYWORD : TT_IDENTIFIER,
                    .value = value,
                    .index = start,
                    .line = line,
                    .col = col - len, 
                    .len = len
                };

                if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
                    free(value); 
                    return NULL;
                }


            }
            else if (isdigit(source[i]) || source[i] == '.') { 
                goto parse_number;
            }
            else { 
                fprintf(stderr, "%s:%zu:%zu: Unexpected character '%c'.\n", file, line, col, source[i]);
                freeTokens(tokens, nTokens);
                return NULL;
            }
            break;
        } 
        } 
    } 

    Token eof_token;

    eof_token.type = TT_EOF;
    eof_token.value = NULL;
    eof_token.index = i;
    eof_token.col = col;
    eof_token.line = line;
    eof_token.len = 0;

    if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, eof_token)) {
        return NULL;
    }

    return tokens;

parse_number:
    {
        size_t start = i;
        bool hasDot = false;

        while (isdigit(source[i]) || source[i] == '.') {
            if (source[i] == '.') {
                if (hasDot) { 
                    fprintf(stderr, "%s:%zu:%zu: Malformed float.\n", file, line, col);
                    freeTokens(tokens, nTokens);
                    return NULL;
                }
                hasDot = true;
            }
            i++;
        }

        size_t len = i - start;
        char* value = malloc(len + 1);

        if (!value) {
            fprintf(stderr, "%s:%zu:%zu: Memory allocation failed.\n", file, line, col);
            freeTokens(tokens, nTokens);
            return NULL;
        }

        strncpy(value, source + start, len);
        value[len] = '\0';

        Token token = {
            .type = hasDot ? TT_FLOAT : TT_INT,
            .value = value,
            .index = start,
            .col = col,
            .line = line,
            .len = len
        };

        if (!appendToken(&tokens, &sTokens, &nTokens, file, line, col, token)) {
            free(value);
            freeTokens(tokens, nTokens);
            return NULL;
        }

        col += len;
        
    }



#ifdef DEBUG
const char* tokenTypeAsString(Token token) {
    switch (token.type) {
    case TT_EOF:
        return "EOF";
    case TT_IDENTIFIER:
        return "IDENTIFIER";
    case TT_KEYWORD:
        return "KEYWORD";
    case TT_INT:
        return "INT";
    case TT_FLOAT:
        return "FLOAT";
    case TT_STRING:
        return "STRING";
    case TT_CHAR:
        return "CHAR";
    case TT_ADD:
        return "ADD";
    case TT_SUB:
        return "SUB";
    case TT_MUL:
        return "MUL";
    case TT_DIV:
        return "DIV";
    case TT_MOD:
        return "MOD";
    case TT_POW:
        return "POW";
    case TT_XOR:
        return "XOR";
    case TT_NOT:
        return "NOT";
    case TT_INC:
        return "INC";
    case TT_DEC:
        return "DEC";
    case TT_LSH:
        return "LSH";
    case TT_RSH:
        return "RSH";
    case TT_BNOT:
        return "BNOT";
    case TT_BXOR:
        return "BXOR";
    case TT_BAND:
        return "BAND";
    case TT_BOR:
        return "BOR";
    case TT_LT:
        return "LT";
    case TT_GT:
        return "GT";
    case TT_LTE:
        return "LTE";
    case TT_GTE:
        return "GTE";
    case TT_EQ:
        return "EQ";
    case TT_NEQ:
        return "NEQ";
    case TT_AND:
        return "AND";
    case TT_OR:
        return "OR";
    case TT_ASSIGN:
        return "ASSIGN";
    case TT_ADDEQ:
        return "ADDEQ";
    case TT_SUBEQ:
        return "SUBEQ";
    case TT_MULEQ:
        return "MULEQ";
    case TT_DIVEQ:
        return "DIVEQ";
    case TT_MODEQ:
        return "MODEQ";
    case TT_LSHEQ:
        return "LSHEQ";
    case TT_RSHEQ:
        return "RSHEQ";
    case TT_ANDEQ:
        return "TT_ANDEQ";
    case TT_OREQ:
        return "TT_OREQ";
    case TT_XOREQ:
        return "TT_XOREQ";
    case TT_LPAREN:
        return "LPAREN";
    case TT_RPAREN:
        return "RPAREN";
    case TT_LBRACKET:
        return "LBRACKET";
    case TT_RBRACKET:
        return "RBRACKET";
    case TT_LBRACE:
        return "LBRACE";
    case TT_RBRACE:
        return "RBRACE";
    case TT_SEMICOLON:
        return "SEMICOLON";
    case TT_COLON:
        return "COLON";
    case TT_DOT:
        return "DOT";
    case TT_COMMA:
        return "COMMA";
    case TT_ARROW:
        return "ARROW";
    case TT_ELLIPSIS:
        return "ELLIPSIS";
    }
    return "UNKNOWN";
}
#endif /* DEBUG */

