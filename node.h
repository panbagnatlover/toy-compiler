/* AST nodes. */

#ifndef H_NODE
#define H_NODE

#include "type.h"
#include "array.h"
#include "symbol.h"

#define DEFAULT_NODE_ARR_SIZE 8

/* Operators for intermediate representation */

typedef enum {
    O_PLU,
    O_MIN,
    O_MUL,
    O_DIV,
    O_ACC
} Op;

typedef enum {
    U_NEG,
    U_NOT,
    U_CAST_BOOL,
    U_CAST_INT,
    U_CAST_FLOAT,
    U_NOT_ZERO
} Unary;

typedef enum {
    R_GEQ,
    R_GE,
    R_LEQ,
    R_LE,
    R_EQ,
    R_NEQ
} Rel;

typedef enum {
    L_AND,
    L_OR
} Logic;

/* AST nodes */

typedef struct Node {
    enum {
        /* Statements */
        N_BLOCK = 1,
        N_DECL,
        N_BREAK,
        N_ASSIGN,
        N_IF,
        N_WHILE,
        N_DO,
        /* Expressions */
        N_LOC,
        N_LOGIC, // Bool nodes could also represent control flow
        N_REL,
        N_UNARY,
        N_ARITH,
        /* Constants */
        N_ID,
        N_NUM,
        N_FLO,
        N_BOOL
    } node_id;

    Type *type;  // Used for type checking for expr nodes
    int line;  // Line number in src code associated with the node

    union {
        struct {
            Op op;
            struct Node *left;
            struct Node *right;
        } arith;

        struct {
            Logic op;
            struct Node *left;
            struct Node *right;
        } logic;

        struct {
            Unary op;
            struct Node *child;
        } unary;

        struct {
            Rel op;
            struct Node *left;
            struct Node *right;
        } rel;

        struct {
            struct Node *left;
            struct Node *ind;
            struct Node *base;
        } loc;

        struct {
            int value;
        } num;

        struct {
            float value;
        } flo;

        struct {
            bool value;
        } boo;

        struct {
            char *name;
            Symbol *symbol;
        } id;

        struct {
            Array *stmts;
            Array *decls;
        } block;

        struct {
            struct Node *cond;
            struct Node *stmt;
        } s_do;

        struct {
            struct Node *cond;
            struct Node *stmt;
        } s_while;

        struct {
            struct Node *cond;
            struct Node *stmt;
            struct Node *estmt;
        } s_if;

        struct {
            Type *type;
            struct Node *id;
        } decl;

        struct {
            struct Node *lvalue;
            struct Node *rvalue;
        } assign;
    };
} Node;

#endif
