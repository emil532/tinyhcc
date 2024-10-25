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

#ifdef TRANSPILER
char *operatorFromToken(Token token) {
    switch (token.type) {
        case TT_ADD: return "+";
        case TT_SUB: return "-";
        case TT_MUL: return "*";
        case TT_DIV: return "/";
        case TT_MOD: return "%";
        case TT_POW: return "`";
        case TT_NOT: return "!";
        case TT_XOR: return "^^";
        case TT_INC: return "++";
        case TT_DEC: return "--";
        case TT_LSH: return "<<";
        case TT_RSH: return ">>";
        case TT_BNOT: return "~";
        case TT_BXOR: return "^";
        case TT_BAND: return "&";
        case TT_BOR: return "|";
        case TT_LT: return "<";
        case TT_GT: return ">";
        case TT_LTE: return "<=";
        case TT_GTE: return ">=";
        case TT_EQ: return "==";
        case TT_NEQ: return "!=";
        case TT_AND: return "&&";
        case TT_OR: return "||";
        case TT_ASSIGN: return "=";
        case TT_ADDEQ: return "+=";
        case TT_SUBEQ: return "-=";
        case TT_MULEQ: return "*=";
        case TT_DIVEQ: return "/=";
        case TT_MODEQ: return "%=";
        case TT_LSHEQ: return "<<=";
        case TT_RSHEQ: return ">>=";
        case TT_ANDEQ: return "&=";
        case TT_OREQ: return "|=";
        case TT_XOREQ: return "^=";
        case TT_DOT: return ".";
        case TT_ARROW: return "->";
        default: return "UNKNOWN";
    }
}

char *regAsString(Register reg) {
    switch (reg) {
        case NONE:
        case AUTO:
            return NULL;
        case REG_RAX: return "RAX";
        case REG_RBX: return "RBX";
        case REG_RCX: return "RCX";
        case REG_RDX: return "RDX";
        case REG_RSI: return "RSI";
        case REG_RDI: return "RDI";
        case REG_RBP: return "RBP";
        case REG_RSP: return "RSP";
        case REG_R8: return "R8";
        case REG_R9: return "R9";
        case REG_R10: return "R10";
        case REG_R11: return "R11";
        case REG_R12: return "R12";
        case REG_R13: return "R13";
        case REG_R14: return "R14";
        case REG_R15: return "R15";
        case REG_EAX: return "EAX";
        case REG_EBX: return "EBX";
        case REG_ECX: return "ECX";
        case REG_ESP: return "ESP";
        case REG_EBP: return "EBP";
        case REG_EDI: return "EDI";
        case REG_ESI: return "ESI";
        case REG_EDX: return "EDX";
        case REG_AX: return "AX";
        case REG_BX: return "BX";
        case REG_CX: return "CX";
        case REG_SP: return "SP";
        case REG_BP: return "BP";
        case REG_DI: return "DI";
        case REG_SI: return "SI";
        case REG_DX: return "DX";
        case REG_AH: return "AH";
        case REG_AL: return "AL";
        case REG_BH: return "BH";
        case REG_BL: return "BL";
        case REG_CH: return "CH";
        case REG_CL: return "CL";
        case REG_SPL: return "SPL";
        case REG_BPL: return "BPL";
        case REG_DIL: return "DIL";
        case REG_SIL: return "SIL";
        case REG_DH: return "DH";
        case REG_DL: return "DL";
        case REG_XMM0: return "XMM0";
        case REG_XMM1: return "XMM1";
        case REG_XMM2: return "XMM2";
        case REG_XMM3: return "XMM3";
        case REG_XMM4: return "XMM4";
        case REG_XMM5: return "XMM5";
        case REG_XMM6: return "XMM6";
        case REG_XMM7: return "XMM7";
    }
}

void printTypedVariable(Type type, Token name) {
    if (!(type.qualifiers & FUNCTION)) {
        if (type.qualifiers & STATIC) printf("static ");
        if (type.qualifiers & PUBLIC) printf("public ");
        if (type.qualifiers & PRIVATE) printf("private ");
        if (type.qualifiers & EXTERN) printf("extern ");
        printf("%s ", type.type.base);
        for (size_t i = 0; i < type.ptrDepth; i++)
            printf("*");
        printf("%s", name.value);
        for (size_t i = 0; i < type.arrayDepth; i++)
            printf("[%zu]", type.arraySizes[i]);
        return;
    }
    Type *stack = malloc(sizeof(Type));
    stack[0] = type;
    size_t depth = 0;
    while ((stack[depth].qualifiers & FUNCTION) && (stack[depth].type.returnType->qualifiers & FUNCTION)) {
        stack = realloc(stack, (depth + 2) * sizeof(Type));
        stack[depth + 1] = *stack[depth].type.returnType;
        depth += 1;
    }
    if (stack[depth].qualifiers & STATIC) printf("static ");
    if (stack[depth].qualifiers & PUBLIC) printf("public ");
    if (stack[depth].qualifiers & PRIVATE) printf("private ");
    if (stack[depth].qualifiers & EXTERN) printf("extern ");
    printf("%s", stack[depth].type.base);
    for (size_t i = 0; i < stack[depth].ptrDepth; i++)
        printf("*");
    for (size_t i = depth; i >= 0; i--) {
        printf("(");
        for (size_t j = 0; j < stack[i].ptrDepth; j++)
            printf("*");
        if (stack[i].qualifiers & STATIC) printf("static ");
        if (stack[i].qualifiers & PUBLIC) printf("public ");
        if (stack[i].qualifiers & PRIVATE) printf("private ");
        if (stack[i].qualifiers & EXTERN) printf("extern ");
    }
    printf("%s", name.value);
    for (size_t i = 0; i < type.arrayDepth; i++)
        printf("[%zu]", type.arraySizes[i]);
    for (size_t i = 0; i < depth + 1; i++) {
        printf(")(");
        for (size_t j = 0; j < stack[i].nParameters; j++) {
            printTypedVariable(stack[i].parameters[j]->type, stack[i].parameters[j]->name);
            if (stack[i].parameters[j]->initializer != NULL) {
                printf(" = ");
                printNode(stack[i].parameters[j]->initializer, 0);
            }
            if (j < stack[i].nParameters - 1)
                printf(", ");
        }
        if (stack[i].qualifiers & VARARG) {
            if (stack[i].nParameters > 0)
                printf(", ");
            printf("...");
        }
        printf(")");
    }
}

void printNode(Node *node, size_t depth) {
    switch (node->type) {
        case NT_INT:
        case NT_FLOAT: {
            printf("%s", ((ValueNode*)node->node)->value.value);
        } break;
        case NT_STRING: {
            printf("\"%s\"", ((ValueNode*)node->node)->value.value);
        } break;
        case NT_CHAR: {
            printf("'%s'", ((ValueNode*)node->node)->value.value);
        } break;
        case NT_ASSIGN:
        case NT_BINOP: {
            BinaryOperationNode *binOp = (BinaryOperationNode*)node->node;
            printf("(");
            printNode(binOp->lhs, 0);
            printf(" %s ", operatorFromToken(binOp->op));
            printNode(binOp->rhs, 0);
            printf(")");
        } break;
        case NT_UNARYOP: {
            UnaryOperationNode *unOp = (UnaryOperationNode*)node->node;
            printf("(");
            printf("%s", operatorFromToken(unOp->op));
            printNode(unOp->value, 0);
            printf(")");
        } break;
        case NT_VARACCESS: {
            VariableAccessNode *varAccess = (VariableAccessNode*)node->node;
            printf("%s", varAccess->name.value);
        } break;
        case NT_VARDECL: {
            VariableDeclerationNode *varDecl = (VariableDeclerationNode*)node->node;
            printTypedVariable(varDecl->type, varDecl->name);
            if (varDecl->initializer != NULL) {
                printf(" = ");
                printNode(varDecl->initializer, 0);
            }
        } break;
        case NT_FUNCCALL: {
            FunctionCallNode *funcCall = (FunctionCallNode*)node->node;
            printf("(");
            printNode(funcCall->function, 0);
            printf("(");
            for (size_t i = 0; i < funcCall->nArguments; i++) {
                printNode(funcCall->arguments[i], 0);
                if (i < funcCall->nArguments - 1)
                    printf(", ");
            }
            printf("))");
        } break;
        case NT_FUNCDECL: {
            FunctionDeclerationNode *funcDecl = (FunctionDeclerationNode*)node->node;
            Type type = funcDecl->type;
            Token name = funcDecl->name;
            Type *stack = malloc(sizeof(Type));
            stack[0] = type;
            size_t depth = 0;
            while ((stack[depth].qualifiers & FUNCTION) && (stack[depth].type.returnType->qualifiers & FUNCTION)) {
                stack = realloc(stack, (depth + 2) * sizeof(Type));
                stack[depth + 1] = *stack[depth].type.returnType;
                depth += 1;
            }
            if (stack[depth].qualifiers & STATIC) printf("static ");
            if (stack[depth].qualifiers & PUBLIC) printf("public ");
            if (stack[depth].qualifiers & PRIVATE) printf("private ");
            if (stack[depth].qualifiers & EXTERN) printf("extern ");
            printf("%s", stack[depth].type.base);
            for (size_t i = 0; i < stack[depth].ptrDepth; i++)
                printf("*");
            for (size_t i = depth; i > 0; i--) {
                printf("(");
                for (size_t j = 0; j < stack[i].ptrDepth; j++)
                    printf("*");
                if (stack[i].qualifiers & STATIC) printf("static ");
                if (stack[i].qualifiers & PUBLIC) printf("public ");
                if (stack[i].qualifiers & PRIVATE) printf("private ");
                if (stack[i].qualifiers & EXTERN) printf("extern ");
            }
            printf("%s", name.value);
            for (size_t i = 0; i < type.arrayDepth; i++)
                printf("[%zu]", type.arraySizes[i]);
            for (size_t i = 0; i < depth + 1; i++) {
                if (i > 0) printf(")");
                printf("(");
                for (size_t j = 0; j < stack[i].nParameters; j++) {
                    printTypedVariable(stack[i].parameters[j]->type, stack[i].parameters[j]->name);
                    if (stack[i].parameters[j]->initializer != NULL) {
                        printf(" = ");
                        printNode(stack[i].parameters[j]->initializer, 0);
                    }
                    if (j < stack[i].nParameters - 1)
                        printf(", ");
                }
                if (stack[i].qualifiers & VARARG) {
                    if (stack[i].nParameters > 0)
                        printf(", ");
                    printf("...");
                }
                printf(")");
            }
            printf(" ");
            Node tmp = (Node) {
                .type = NT_COMPOUND,
                .node = funcDecl->body
            };
            printNode(&tmp, depth + 1);
        } break;
        case NT_ARRAYACCESS: {
            ArrayAccessNode *access = (ArrayAccessNode*)node->node;
            printf("(");
            printNode(access->array, 0);
            printf("[");
            printNode(access->index, 0);
            printf("]");
            printf(")");
        } break;
        case NT_ACCESS: {
            AccessNode *access = (AccessNode*)node->node;
            printf("(");
            printNode(access->object, 0);
            printf("%s%s)", operatorFromToken(access->op), access->member.value);
        } break;
        case NT_FOR: {
            ForNode *forLoop = (ForNode*)node->node;
            printf("for (");
            if (forLoop->initializer)
                printNode(forLoop->initializer, 0);
            printf(";");
            if (forLoop->condition)
                printNode(forLoop->condition, 0);
            printf(";");
            if (forLoop->increment)
                printNode(forLoop->increment, 0);
            printf(") ");
            printNode(forLoop->body, depth);
        } break;
        case NT_WHILE: {
            WhileNode *whileLoop = (WhileNode*)node->node;
            printf("while (");
            printNode(whileLoop->condition, 0);
            printf(") ");
            printNode(whileLoop->body, depth);
        } break;
        case NT_IF: {
            IfNode* ifStatement = (IfNode*)node->node;
            printf("if (");
            printNode(ifStatement->conditions[0], 0);
            printf(") ");
            printNode(ifStatement->bodies[0], depth);
            for (size_t i = 1; i < ifStatement->nCases; i++) {
                printf(" else if (");
                printNode(ifStatement->conditions[i], 0);
                printf(") ");
                printNode(ifStatement->bodies[i], depth);
            }
            if (ifStatement->elseCase) {
                printf(" else ");
                printNode(ifStatement->elseCase, depth);
            }
        }
        case NT_SWITCH: {
            printf("TODO: NT_SWITCH");
        } break;
        case NT_GOTO: {
            printf("goto %s", ((GotoNode*)node->node)->label.value);
        } break;
        case NT_LABEL: {
            printf("%s:", ((LabelNode*)node->node)->name.value);
        } break;
        case NT_BREAK: {
            printf("break");
        } break;
        case NT_TRY: {
            TryNode *try = (TryNode*)node->node;
            printf("try ");
            printNode(try->body, depth);
            printf(" catch ");
            printNode(try->catchBody, depth);
        } break;
        case NT_CLASS: {
            TypeNode *type = (TypeNode*)node->node;
            printf("class %s {\n", type->name.value);
            for (size_t i = 0; i < type->nFields; i++) {
                for (size_t j = 0; j < depth; j++)
                    printf("  ");
                Node tmp = (Node) {
                    .type = NT_VARDECL,
                    .node = type->fields[i]
                };
                printNode(&tmp, 0);
                printf(";");
            }
            printf("}");
        } break;
        case NT_UNION: {
            TypeNode *type = (TypeNode*)node->node;
            printf("union %s {\n", type->name.value);
            for (size_t i = 0; i < type->nFields; i++) {
                for (size_t j = 0; j < depth; j++)
                    printf("  ");
                Node tmp = (Node) {
                    .type = NT_VARDECL,
                    .node = type->fields[i]
                };
                printNode(&tmp, 0);
                printf(";");
            }
            printf("}");
        } break;
        case NT_COMPOUND: {
            CompoundNode *compound = (CompoundNode*)node->node;
            printf("{\n");
            for (size_t i = 0; i < compound->nStatements; i++) {
                for (size_t j = 0; j < depth; j++)
                    printf("  ");
                printNode(compound->statements[i], depth + 1);
                if (compound->statements[i]->type != NT_LABEL)
                    printf(";");
            }
            printf("}");
        } break;
    }
}
#endif /* TRANSPILER */
