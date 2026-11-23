#ifndef H_ALIASMAP
#define H_ALIASMAP

#include "arena.h"
#include "memory.h"
#include "var.h"

#define VAL_K_THRES 12
#define MEM_K_THRES 10

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
} PTType;

typedef enum {
    NO_ALIAS = 0,
    MUST_ALIAS,
    MAY_ALIAS
} AliasType;

typedef struct {
    ValType val_type;
    Array *val_set;  // Capped array of vals, 0 = NAI, 1 = ALL_VAL, 2-11 = other ints
} ValSet;

typedef struct {
    PTType pt_type;
    void *pt_set;  // Capped array of mem state objs, sorted by base, or a single pointer
} PointToSet;

typedef struct {
    Memory mem;
    PointToSet pt_set;
    ValSet val_set;
} MemStateObj;

typedef struct {
    Arena *objs;
    /* TODO: some kind of data structure allowing for
     * sorted memory state object pointers. */
    Array *map;
} AliasState;

MemStateObj *get_mem(Var *var, Val offset, AliasState *state);
AliasState *alias_analysis(Array *blocks);

#endif
