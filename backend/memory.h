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
    enum { NO_VAL = 0, VAL, NAI, ALL_VAL } type;
    size_t value;
} Val;

typedef struct {
    enum { NO_MEM = 0, MEM, ERR_MEM, ALL_MEM } type;
    Var *base; // If NULL, NULL.
    Val offset;
} Memory;

typedef enum {
    NO_VAL = 0,
    MUST_VAL,
    MUST_NAI,
    MAY_VAL,
    MAY_NAI,
    MAY_ALL_VAL,
    MAY_ALL_NAI
} ValType;

typedef enum {
    NO_MEM = 0,
    MUST_MEM,
    MAY_MEM,
    MAY_ALL_MEM
} AliasType;

bool eq_val(Val a, Val b);
bool eq_mem(Memory a, Memory b);

#endif
