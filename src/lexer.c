/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lexer.h"

static inline void appendToken(Token **tokens, size_t *stokens, size_t *ntokens, Token token) {
    if (*stokens == *ntokens) {
        *stokens *= 2;
        *tokens = realloc(*tokens, *stokens * sizeof(Token));
    }
    (*tokens)[(*ntokens)++] = token;
}

static const char *keywords[] = {
    "if", "else", "while", "for", "switch", "case", "asm", "try", "catch", "throw", "break", "goto", "class", "union", NULL
};

Token *tokenize(const char *source, const char *file) {
    Token* tokens = malloc(128 * sizeof(Token)); /* For performance reasons we assume files will have 128 or more tokens */
    size_t stokens = 128;
    size_t ntokens = 0;
    size_t i = 0;
    size_t line = 1;
    size_t col = 1;
    while (source[i]) {
        switch (source[i]) {
            case '\t':
            case '\r':
            case ' ':
                break;
            case '\n':
                line += 1;
                col = 0;
                break;
            case '+': {
                if (source[i + 1] && source[i + 1] == '+') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_INC,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_ADDEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_ADD,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '-': {
                if (source[i + 1] && source[i + 1] == '-') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_DEC,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_SUBEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '>') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_ARROW,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_SUB,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '*': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_MULEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_MUL,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '/': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_DIVEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '/') {
                    while (source[i] && source[i] != '\n') i += 1;
                    break;
                } else if (source[i + 1] && source[i + 1] == '*') {
                    i += 1; /* Prevents TT_DIV TT_MUL TT_DIV from being seen as a full block comment */
                    while (source[++i]) {
                        if (source[i] && source[i + 1] && source[i] == '*' && source[i + 1] == '/')
                            break;
                    }
                    if (!source[i]) {
                        fprintf(stderr, "%s:%zu:%zu: Reached EOF while parsing block comment.\n", file, line, col);
                        free(tokens);
                        return NULL;
                    }
                    i += 1;
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_DIV,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '%': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_MODEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_MOD,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '<': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_LTE,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if(source[i + 1] && source[i + 2] && source[i + 1] == '<' && source[i + 2] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_LSHEQ,
                        .value = NULL,
                        .file = file,
                        .index = i,
                        .col = col,
                        .line = line,
                        .len = 3
                    });
                    i += 2;
                    col += 2;
                    break;
                } else if (source[i + 1] && source[i + 1] == '<') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_LSH,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_LT,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '>': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_GTE,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if(source[i + 1] && source[i + 2] && source[i + 1] == '>' && source[i + 2] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_RSHEQ,
                        .value = NULL,
                        .file = file,
                        .index = i,
                        .col = col,
                        .line = line,
                        .len = 3
                    });
                    i += 2;
                    col += 2;
                    break;
                } else if (source[i + 1] && source[i + 1] == '>') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_RSH,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_GT,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '~': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_BNOT,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '^': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_XOREQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '^') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_XOR,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_BXOR,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '`': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_POW,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '&': {
                if (source[i + 1] && source[i + 1] == '&') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_AND,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_ANDEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_BAND,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '|': {
                if (source[i + 1] && source[i + 1] == '|') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_OR,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                } else if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_OREQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_BOR,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '=': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_EQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_ASSIGN,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '!': {
                if (source[i + 1] && source[i + 1] == '=') {
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_NEQ,
                        .value = NULL,
                        .file = file,
                        .index = i++,
                        .col = col++,
                        .line = line,
                        .len = 2
                    });
                    break;
                }
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_NOT,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '(': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_LPAREN,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case ')': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_RPAREN,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '{': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_LBRACE,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '}': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_RBRACE,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '[': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_LBRACKET,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case ']': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_RBRACKET,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case ';': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_SEMICOLON,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case ':': {
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_COLON,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            case '.': {
                if (source[i + 1] >= '0' && source[i + 1] <= '9') goto parse_number;
                appendToken(&tokens, &stokens, &ntokens, (Token) {
                    .type = TT_DOT,
                    .value = NULL,
                    .file = file,
                    .index = i,
                    .col = col,
                    .line = line,
                    .len = 1
                });
            } break;
            default: {
                if (
                    (source[i] >= 'a' && source[i] <= 'z') ||
                    (source[i] >= 'A' && source[i] <= 'Z') ||
                    source[i] == '_') {
                    size_t start = i;
                    while (source[i] && (
                        (source[i] >= 'a' && source[i] <= 'z') ||
                        (source[i] >= 'A' && source[i] <= 'Z') ||
                        (source[i] >= '0' && source[i] <= '9') ||
                        source[i] == '_')
                    ) i += 1;
                    size_t len = (i - start);
                    char *value = malloc(len + 1);
                    memcpy(value, source + start, len);
                    value[len] = 0;
                    bool isKeyword = false;
                    for (size_t j = 0; keywords[j]; j++)
                        isKeyword = isKeyword || !strcmp(value, keywords[j]);
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = isKeyword ? TT_KEYWORD : TT_IDENTIFIER,
                        .value = value,
                        .file = file,
                        .index = start,
                        .col = col,
                        .line = line,
                        .len = len
                    });
                    col += len;
                    continue;
                } else if ((source[i] >= '0' && source[i] <= '9') || source[i] == '.') {
                    parse_number:
                    size_t start = i;
                    bool hasDot = false;
                    while (source[i] && (
                        (source[i] >= '0' && source[i] <= '9') ||
                        source[i] == '.')
                    ) {
                        if (source[i] == '.' && !hasDot) hasDot = true;
                        else if (source[i] == '.' && hasDot) {
                            fprintf(stderr, "%s:%zu:%zu: Malformed float.\n", file, line, col);
                            free(tokens);
                            return NULL;
                        }
                        i += 1;
                    }
                    size_t len = (i - start);
                    char *value = malloc(len + 1);
                    memcpy(value, source + start, len);
                    value[len] = 0;
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = hasDot ? TT_FLOAT : TT_INT,
                        .value = value, /* Guaranteed to be a valid int or float */
                        .file = file,
                        .index = start,
                        .col = col,
                        .line = line,
                        .len = len
                    });
                    col += len;
                    continue;
                } else if (source[i] == '"') {
                    size_t start = ++i; /* Skip the " */
                    while (source[i] && source[i] != '"') i += 1;
                    size_t len = (i - start);
                    char *value = malloc(len + 1);
                    memcpy(value, source + start, len);
                    value[len] = 0;
                    appendToken(&tokens, &stokens, &ntokens, (Token) {
                        .type = TT_STRING,
                        .value = value,
                        .file = file,
                        .index = start,
                        .col = col,
                        .line = line,
                        .len = len
                    });
                    col += len + 1;
                } else {
                    fprintf(stderr, "%s:%zu:%zu: Unexpected character '%c'.\n", file, line, col, source[i]);
                    free(tokens);
                    return NULL;
                }
            } break;
        }
        i += 1;
        col += 1;
    }
    appendToken(&tokens, &stokens, &ntokens, (Token) {
        .type = TT_EOF,
        .value = NULL,
        .file = file,
        .index = i,
        .col = col,
        .line = line,
        .len = 1
    });
    return tokens;
}

#ifdef DEBUG
const char *tokenTypeAsString(Token token) {
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
        case TT_ARROW:
            return "ARROW";
        }
    return "UNKNOWN";
}
#endif