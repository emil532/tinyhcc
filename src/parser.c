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

Node *parseExpression(ParserContext *ctx);

Node *parseLiteralExpression(ParserContext *ctx) {
    if (ISCURRENTTOKENTYPE(ctx, TT_INT)) {
        ValueNode *value = malloc(sizeof(ValueNode));
        value->value = CURRENTTOKEN(ctx);
        Node *intNode = malloc(sizeof(Node));
        intNode->type = NT_INT;
        intNode->node = value;
        advance(ctx);
        return intNode;
    } else if (ISCURRENTTOKENTYPE(ctx, TT_FLOAT)) {
        ValueNode *value = malloc(sizeof(ValueNode));
        value->value = CURRENTTOKEN(ctx);
        Node *fltNode = malloc(sizeof(Node));
        fltNode->type = NT_FLOAT;
        fltNode->node = value;
        advance(ctx);
        return fltNode;
    } else if (ISCURRENTTOKENTYPE(ctx, TT_STRING)) {
        ValueNode *value = malloc(sizeof(ValueNode));
        value->value = CURRENTTOKEN(ctx);
        Node *strNode = malloc(sizeof(Node));
        strNode->type = NT_STRING;
        strNode->node = value;
        advance(ctx);
        return strNode;
    } else if (ISCURRENTTOKENTYPE(ctx, TT_CHAR)) {
        ValueNode *value = malloc(sizeof(ValueNode));
        value->value = CURRENTTOKEN(ctx);
        Node *chrNode = malloc(sizeof(Node));
        chrNode->type = NT_CHAR;
        chrNode->node = value;
        advance(ctx);
        return chrNode;
    } else if (ISCURRENTTOKENTYPE(ctx, TT_IDENTIFIER)) {
        VariableAccessNode *access = malloc(sizeof(VariableAccessNode));
        access->name = CURRENTTOKEN(ctx);
        Node *accessNode = malloc(sizeof(Node));
        accessNode->type = NT_VARACCESS;
        accessNode->node = access;
        advance(ctx);
        return accessNode;
    } else if (ISCURRENTTOKENTYPE(ctx, TT_LPAREN)) {
        advance(ctx);
        Node *expression = parseExpression(ctx);
        if (expression == NULL) {
            /* TODO: Error message here */
            return NULL;
        }
        if (!ISCURRENTTOKENTYPE(ctx, TT_RPAREN)) {
            /* TODO: Error message here */
            return NULL;
        }
        advance(ctx);
        return expression;
    }

    /* TODO: Error message here */
    return NULL;
}

Node *parseAccessExpression(ParserContext *ctx) {
    Node *access = parseLiteralExpression(ctx);
    while (
        ISCURRENTTOKENTYPE(ctx, TT_LPAREN) || ISCURRENTTOKENTYPE(ctx, TT_LBRACKET) ||
        ISCURRENTTOKENTYPE(ctx, TT_DOT)    || ISCURRENTTOKENTYPE(ctx, TT_ARROW)
    ) {
        if (ISCURRENTTOKENTYPE(ctx, TT_LPAREN)) {
            advance(ctx);
            Node **arguments = NULL;
            size_t nArguments = 0;
            if (!ISCURRENTTOKENTYPE(ctx, TT_RPAREN)) {
                do {
                    if (ISCURRENTTOKENTYPE(ctx, TT_COMMA))
                        advance(ctx);
                    Node *expression = parseExpression(ctx);
                    if (expression == NULL) {
                        /* TODO: Error message here */
                        return NULL;
                    }
                    arguments = realloc(arguments, (nArguments + 1) * sizeof(Node*));
                    arguments[nArguments++] = expression;
                } while (ISCURRENTTOKENTYPE(ctx, TT_COMMA));
            }
            if (!ISCURRENTTOKENTYPE(ctx, TT_RPAREN)) {
                /* TODO: Error message here */
                return NULL;
            }
            advance(ctx);
            FunctionCallNode *funcCall = malloc(sizeof(FunctionCallNode));
            funcCall->function = access;
            funcCall->arguments = arguments;
            funcCall->nArguments = nArguments;
            access = malloc(sizeof(Node));
            access->type = NT_FUNCCALL;
            access->node = funcCall;
        } else if (ISCURRENTTOKENTYPE(ctx, TT_LBRACKET)) {
            advance(ctx);
            Node *index = parseExpression(ctx);
            if (index == NULL) {
                /* TODO: Error message here */
                return NULL;
            }
            if (!ISCURRENTTOKENTYPE(ctx, TT_RBRACKET)) {
                /* TODO: Error message here */
                return NULL;
            }
            advance(ctx);
            ArrayAccessNode *arrayAccess = malloc(sizeof(ArrayAccessNode));
            arrayAccess->array = access;
            arrayAccess->index = index;
            access = malloc(sizeof(Node));
            access->type = NT_ARRAYACCESS;
            access->node = arrayAccess;
        } else {
            Token op = CURRENTTOKEN(ctx);
            advance(ctx);
            if (!ISCURRENTTOKENTYPE(ctx, TT_IDENTIFIER)) {
                /* TODO: Error message here */
                return NULL;
            }
            Token member = CURRENTTOKEN(ctx);
            advance(ctx);
            AccessNode* acc = malloc(sizeof(AccessNode));
            acc->object = access;
            acc->op = op;
            acc->member = member;
            access = malloc(sizeof(Node));
            access->type = NT_ACCESS;
            access->node = acc;
        }
    }
    return access;
}

Node *parseUnaryExpression(ParserContext *ctx) {
    if (ISCURRENTTOKENTYPE(ctx, TT_SUB) || ISCURRENTTOKENTYPE(ctx, TT_MUL)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *expression = parseUnaryExpression(ctx);
        if (expression == NULL) {
            /* TODO: Error message here */
            return NULL;
        }
        UnaryOperationNode *unOp = malloc(sizeof(UnaryOperationNode));
        unOp->op = op;
        unOp->value = expression;
        Node *res = malloc(sizeof(Node));
        res->type = NT_UNARYOP;
        res->node = unOp;
    }
    return parseAccessExpression(ctx);
}

Node *parseFactorExpression(ParserContext *ctx) {
    Node *lhs = parseUnaryExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_POW) || ISCURRENTTOKENTYPE(ctx, TT_LSH) || ISCURRENTTOKENTYPE(ctx, TT_RSH)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseUnaryExpression(ctx);
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

Node *parseTermExpression(ParserContext *ctx) {
    Node *lhs = parseFactorExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_MUL) || ISCURRENTTOKENTYPE(ctx, TT_DIV) || ISCURRENTTOKENTYPE(ctx, TT_MOD)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseFactorExpression(ctx);
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

Node *parseBinaryAndExpression(ParserContext *ctx) {
    Node *lhs = parseTermExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_BAND)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseTermExpression(ctx);
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

Node *parseBinaryXorExpression(ParserContext *ctx) {
    Node *lhs = parseBinaryAndExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_BXOR)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseBinaryAndExpression(ctx);
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

Node *parseBinaryOrExpression(ParserContext *ctx) {
    Node *lhs = parseBinaryXorExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_BOR)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseBinaryXorExpression(ctx);
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

Node *parseArithmeticExpression(ParserContext *ctx) {
    Node *lhs = parseBinaryOrExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_ADD)  || ISCURRENTTOKENTYPE(ctx, TT_SUB)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseBinaryOrExpression(ctx);
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

Node *parseComparisonExpression(ParserContext *ctx) {
    Node *lhs = parseArithmeticExpression(ctx);
    while (
        ISCURRENTTOKENTYPE(ctx, TT_LT)  || ISCURRENTTOKENTYPE(ctx, TT_GT) ||
        ISCURRENTTOKENTYPE(ctx, TT_LTE) || ISCURRENTTOKENTYPE(ctx, TT_GTE)
    ) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseArithmeticExpression(ctx);
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

Node *parseEqualityExpression(ParserContext *ctx) {
    Node *lhs = parseComparisonExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_EQ) || ISCURRENTTOKENTYPE(ctx, TT_NEQ)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseComparisonExpression(ctx);
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

Node *parseAndExpression(ParserContext *ctx) {
    Node *lhs = parseEqualityExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_AND)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseEqualityExpression(ctx);
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

Node *parseXorExpression(ParserContext *ctx) {
    Node *lhs = parseAndExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_XOR)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseAndExpression(ctx);
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

Node *parseOrExpression(ParserContext *ctx) {
    Node *lhs = parseXorExpression(ctx);
    while (ISCURRENTTOKENTYPE(ctx, TT_OR)) {
        Token op = CURRENTTOKEN(ctx);
        advance(ctx);
        Node *rhs = parseXorExpression(ctx);
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
        advance(ctx);
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
        if (!ISCURRENTTOKENTYPE(&ctx, TT_SEMICOLON)) {
            /* TODO: Print an error here */
            break;
        }
        advance(&ctx);
        program->statements = realloc(program->statements, (program->nStatements + 1) * sizeof(Node*));
        program->statements[program->nStatements++] = statement;
    }

    AST->type = NT_COMPOUND;
    AST->node = program;
    return AST;
}
