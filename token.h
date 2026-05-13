/* Lexer */

#ifndef H_TOKEN
#define H_TOKEN

#include <stdbool.h>
#include "type.h"
#include "context.h"

typedef enum {
    EOF_TOKEN   = 0,
    ERR_TOKEN   = 1,
    ID          = 2, // string identifier
    NUM         = 3, // int value
    FLO         = 4, // float value
    TRU         = 5, // true value
    FAL         = 6, // false value
    ASG         = 7, // =
    NOT         = 8, // !
    PLU         = 9, // +
    MIN         = 10, // -
    MUL         = 11, // *
    DIV         = 12, // \/
    EQL         = 13, // ==
    NEQ         = 14, // !=
    GE          = 15,  // >
    GEQ         = 16, // >=
    LE          = 17,  // <
    LEQ         = 18, // <=
    AND         = 19, // &&
    OR          = 20,  // ||
    LBR         = 21, // {
    RBR         = 22, // }
    LPR         = 23, // (
    RPR         = 24, // )
    LSQ         = 25, // [
    RSQ         = 26, // ]
    SEM         = 27, // ;
    BASIC       = 28, // int, float, bool
    DO          = 29, // do
    WHILE       = 30, // while
    IF          = 31, // if
    ELSE        = 32, // else
    BREAK       = 33 // break
} TokenID;

typedef struct {
    TokenID token_id;
    int line;
    union {
        Type *type;
        int num_val;
        float flo_val;
        char *word;
    };
} Token;

typedef struct {
    TokenID token_id;
    union {
        char *name;  // Reference to interned string in arena
        Type *type;
    };
} TokenIDLookup;

#endif
