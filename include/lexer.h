/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

typedef enum TokenType {
    TT_EOF,

    /* Identifiers */
    TT_IDENTIFIER,
    TT_KEYWORD,
    TT_INT,
    TT_FLOAT,
    TT_STRING,
    TT_CHAR,

    /* Operations */
    TT_ADD,
    TT_SUB,
    TT_MUL,
    TT_DIV,
    TT_MOD,
    TT_POW,
    TT_NOT,
    TT_XOR,

    TT_INC,
    TT_DEC,

    /* Bitwise */
    TT_LSH,
    TT_RSH,
    TT_BNOT,
    TT_BXOR,
    TT_BAND,
    TT_BOR,

    /* Comparative */
    TT_LT,
    TT_GT,
    TT_LTE,
    TT_GTE,
    TT_EQ,
    TT_NEQ,
    TT_AND,
    TT_OR,

    /* Asignment */
    TT_ASSIGN,
    TT_ADDEQ,
    TT_SUBEQ,
    TT_MULEQ,
    TT_DIVEQ,
    TT_MODEQ,
    TT_LSHEQ,
    TT_RSHEQ,
    TT_ANDEQ,
    TT_OREQ,
    TT_XOREQ,

    /* Braces */
    TT_LPAREN,
    TT_RPAREN,
    TT_LBRACKET,
    TT_RBRACKET,
    TT_LBRACE,
    TT_RBRACE,

    /* Misc. */
    TT_SEMICOLON,
    TT_COLON,
    TT_DOT,
    TT_COMMA,
    TT_ARROW,
    TT_ELLIPSIS
} TokenType;

typedef struct Token {
    TokenType type;
    char *value;
    /* Positional data for error messages */
    size_t index;
    size_t line;
    size_t col;
    size_t len;
} Token;

Token *tokenize(const char *source, const char *file);
void freeTokens(Token *tokens);
#ifdef DEBUG
const char *tokenTypeAsString(Token token);
#endif /* DEBUG */

#endif /* LEXER_H */
