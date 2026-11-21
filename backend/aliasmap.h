#ifndef H_ALIASMAP
#define H_ALIASMAP

#include "arena.h"
#include "dfa.h"
#include "memory.h"
#include "var.h"

#define VAL_K_THRES 12
#define MEM_K_THRES 10

typedef struct {
    ValType val_type;
    Array *val_set;  // Capped array of vals, 0 = NAI, 1 = ALL_VAL, 2-11 = other ints
} ValSet;

typedef struct {
    AliasType alias_type;
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
