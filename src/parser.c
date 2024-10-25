/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lexer.h"
#include "parser.h"

#define ISTOKENTYPE(TOKEN, TYPE) ((TOKEN).type == (TYPE))
#define ISTOKENVALUE(TOKEN, VALUE) (!strcmp((TOKEN).value, (VALUE)))
#define ISTOKEN(TOKEN, TYPE, VALUE) (ISTOKENTYPE((TOKEN), (TYPE)) && ISTOKENVALUE((TOKEN), (VALUE)))
#define ISCURRENTTOKENTYPE(CTX, TYPE) ISTOKENTYPE((CTX)->current, (TYPE))
#define ISCURRENTTOKENVALUE(CTX, VALUE) ISTOKENVALUE((CTX)->current, (VALUE))
#define ISCURRENTTOKEN(CTX, TYPE, VALUE) ISTOKEN((CTX)->current, (TYPE), (VALUE))
#define CURRENTTOKEN(CTX) ((CTX)->current)

/* Order matters, higher up functions take presedence */
Node *parseLiteralExpression(ParserContext *ctx);
Node *parseFunctionCallExpression(ParserContext *ctx);
Node *parseFactorExpression(ParserContext *ctx);
Node *parseTermExpression(ParserContext *ctx);
Node *parseBinaryAndExpression(ParserContext *ctx);
Node *parseBinaryXorExpression(ParserContext *ctx);
Node *parseBinaryOrExpression(ParserContext *ctx);
Node *parseArithmeticExpression(ParserContext *ctx);
Node *parseComparisonExpression(ParserContext *ctx);
Node *parseEqualityExpression(ParserContext *ctx);
Node *parseAndExpression(ParserContext *ctx);
Node *parseXorExpression(ParserContext *ctx);

Node *parseOrExpression(ParserContext *ctx) {
    return NULL;
}

Node *parseAssignmentExpression(ParserContext *ctx) {
    Node *lhs = parseOrExpression(ctx);
    while (
        ISCURRENTTOKENTYPE(ctx, TT_ASSIGN) || ISCURRENTTOKENTYPE(ctx, TT_LSHEQ) ||
        ISCURRENTTOKENTYPE(ctx, TT_RSHEQ)  || ISCURRENTTOKENTYPE(ctx, TT_MULEQ) ||
        ISCURRENTTOKENTYPE(ctx, TT_DIVEQ)  || ISCURRENTTOKENTYPE(ctx, TT_ANDEQ) ||
        ISCURRENTTOKENTYPE(ctx, TT_OREQ)   || ISCURRENTTOKENTYPE(ctx, TT_XOREQ) ||
        ISCURRENTTOKENTYPE(ctx, TT_ADDEQ)  || ISCURRENTTOKENTYPE(ctx, TT_SUBEQ)
    ) {
        Token op = CURRENTTOKEN(ctx);
        Node *rhs = parseOrExpression(ctx);
        BinaryOperationNode* binop = malloc(sizeof(BinaryOperationNode));
        binop->lhs = lhs;
        binop->rhs = rhs;
        binop->op = op;
        lhs = malloc(sizeof(Node));
        lhs->type = NT_BINOP;
        lhs->node = binop;
    }
    return lhs;
}

Node *parseExpression(ParserContext *ctx) {
    return parseAssignmentExpression(ctx);
}

Node *parseStatement(ParserContext *ctx) {
    if (ISCURRENTTOKENTYPE(ctx, TT_KEYWORD)) {
        /* TOOD: Keywords */
        return NULL;
    }
    Node *expression = parseExpression(ctx);
    if (expression == NULL) {
        /* TODO: Print some error to the user */
        return NULL;
    }
    return expression;
}

Node *parse(Token *tokens, const char *file, const char *source) {
    ParserContext ctx = {
        .tokens = tokens,
        .index = -1,
        .file = file,
        .source = source
    };
    advance(&ctx);

    Node *AST = malloc(sizeof(Node));
    CompoundNode *program = malloc(sizeof(CompoundNode));
    program->nStatements = 0;
    program->statements = NULL;

    while (ctx.current.type != TT_EOF) {
        Node *statement = parseStatement(&ctx);
        if (statement == NULL)
            break;
        program->statements = realloc(program->statements, program->nStatements + 1);
        program->statements[program->nStatements++] = statement;
    }

    AST->type = NT_COMPOUND;
    AST->node = program;
    return AST;
}
