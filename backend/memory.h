/* TODO -
 * Introduce a uni-size pointer type in frontend
 * type pointer: size = 8 bytes
 * A variable can have type pointer
 * 
 * In middle end, all non-pointer/array types have pt() = {NAP}.
 * stuff in pt() is Memory. Memory has a base variable + offset or raw addr.
 * Base array assigned a base variable.
 * 
 */
#ifndef H_MEMORY
#define H_MEMORY

#include "stdlib.h"

typedef struct {
    enum {CONST, NAV, ERR_VAL, ALL_VAL} type;
    size_t val;
} Val;

typedef struct {
    enum {MEM, NAP, NULL_MEM, ERR_MEM, ALL_MEM} type;
    Var *base;
    Val offset;
} Memory;

typedef enum {
    NO_VAL = 0,
    MAY_VAL,
    MUST_VAL
} ValType;

typedef enum {
    NO_ALIAS = 0,
    MAY_ALIAS,
    MUST_ALIAS
} AliasType;

#endif
