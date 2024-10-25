/*
 * This file is part of the tinyhcc project.
 * https://github.com/wofflevm/tinyhcc
 * See LICENSE for license.
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum NodeType {
    /* Basic types */
    NT_INT,
    NT_FLOAT,
    NT_STRING,
    NT_CHAR,

    /* Expression types */
    NT_BINOP,
    NT_UNARYOP,

    /* Variables and functions */
    NT_VARACCESS,
    NT_VARDECL,
    NT_ASSIGN,
    NT_FUNCCALL,
    NT_FUNCDECL,

    /* Dereferencing and accessing */
    NT_ARRAYACCESS,
    NT_ACCESS,

    /* Control flow */
    NT_FOR,
    NT_WHILE,
    NT_IF,
    NT_SWITCH,
    NT_GOTO,
    NT_LABEL,
    NT_BREAK,

    /* Error handling */
    NT_TRY,

    /* Types */
    NT_CLASS,
    NT_UNION,

    /* Misc. */
    NT_COMPOUND
} NodeType;

typedef struct Node Node;
typedef struct CompoundNode CompoundNode;
typedef struct VariableDeclerationNode VariableDeclerationNode;

typedef enum Register {
    /* Pseudo */
    NONE, /* Either noreg qualifier or no register qualifier at all */
    AUTO, /* reg qualifier with no provided register */
    /* 64bit */
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RSI,
    REG_RDI,
    REG_RBP,
    REG_RSP,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    /* 32bit */
    REG_EAX,
    REG_EBX,
    REG_ECX,
    REG_ESP,
    REG_EBP,
    REG_EDI,
    REG_ESI,
    REG_EDX,
    /* 16bit */
    REG_AX,
    REG_BX,
    REG_CX,
    REG_SP,
    REG_BP,
    REG_DI,
    REG_SI,
    REG_DX,
    /* 8bit */
    REG_AH,
    REG_AL,
    REG_BH,
    REG_BL,
    REG_CH,
    REG_CL,
    REG_SPL,
    REG_BPL,
    REG_DIL,
    REG_SIL,
    REG_DH,
    REG_DL,
    /* SSE */
    REG_XMM0,
    REG_XMM1,
    REG_XMM2,
    REG_XMM3,
    REG_XMM4,
    REG_XMM5,
    REG_XMM6,
    REG_XMM7
    /* Other registers are not supported for variables */
} Register;

typedef enum Qualifier {
    STATIC       = 1 << 0,
    PRIVATE      = 1 << 1,
    PUBLIC       = 1 << 2,
    EXTERN       = 1 << 3,
    VARARG       = 1 << 8, /* Pseudo-qualifier */
    FUNCTION     = 1 << 9  /* Pseudo-qualifier */
} Qualifier;

typedef struct Type {
    Register reg;
    Qualifier qualifiers;
    size_t ptrDepth;
    size_t *arraySizes;
    size_t arrayDepth;
    VariableDeclerationNode **parameters;
    size_t nParameters;
    union {
        struct Type *returnType;
        char *base;
    } type;
} Type;

/* For ints, floats, strings, and chars */
typedef struct ValueNode {
    Token value;
} ValueNode;

/* Arithmetic, comparisons, class/union accesses, and assignments  */
typedef struct BinaryOperationNode {
    Node *lhs;
    Node *rhs;
    Token op;
} BinaryOperationNode;

/* For negating, notting, dereferencing, and getting the address of values */
typedef struct UnaryOperationNode {
    Node *value;
    Token op;
} UnaryOperationNode;

struct VariableDeclerationNode {
    Type type;
    Token name;
    Node *initializer;
};

typedef struct VariableAccessNode {
    Token name;
} VariableAccessNode;

typedef struct FunctionCallNode {
    Node *function;
    Node **arguments;
    size_t nArguments;
} FunctionCallNode;

typedef struct FunctionDeclerationNode {
    Type type;
    Token name;
    CompoundNode *body;
} FunctionDeclerationNode;

typedef struct ArrayAccessNode {
    Node *array;
    Node *index;
} ArrayAccessNode;

typedef struct AccessNode {
    Node *object;
    Token op;
    Token member;
} AccessNode;

typedef struct ForNode {
    Node *initializer;
    Node *condition;
    Node *increment;
    Node *body;
} ForNode;

typedef struct WhileNode {
    Node *condition;
    Node *body;
} WhileNode;

typedef struct IfNode {
    Node **conditions;
    Node **bodies;
    Node *elseCase;
    size_t nCases;
} IfNode;

typedef struct SwitchNode {
    Node *value;
    Node **cases;
    Node **bodies;
    Node *defaultCase;
    /* For sub-switches */
    size_t startIndex;
    size_t endIndex;
} SwitchNode;

typedef struct GotoNode {
    Token label;
} GotoNode;

typedef struct LabelNode {
    Token name;
} LabelNode;

typedef struct TryNode {
    Node *body;
    Node *catchBody;
} TryNode;

/* For class and union declerations */
typedef struct TypeNode {
    VariableDeclerationNode **fields;
    size_t nFields;
    Token name;
} TypeNode;

struct CompoundNode {
    Node **statements;
    size_t nStatements;
};

struct Node {
    NodeType type;
    void *node;
};

/* --- */

typedef struct ParserContext {
    Token *tokens;
    Token current;
    size_t index;
    /* For printing errors */
    const char *file;
    const char *source;
} ParserContext;

static inline void advance(ParserContext *ctx) {
    ctx->current = ctx->tokens[++ctx->index];
}

Node *parse(Token *tokens, const char *file, const char *source);
#ifdef TRANSPILER
void printNode(Node *node, size_t depth);
#endif /* TRANSPILER */
#endif /* PARSER_H */
