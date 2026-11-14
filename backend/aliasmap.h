#ifndef H_ALIASMAP
#define H_ALIASMAP

#include "arena.h"
#include "dfa.h"
#include "memory.h"

/* If val(x) can take on more than 10 constants,
 * set val(x) = ALL_VAL. Same for pt(x). */
#define VAL_K_THRES 13
#define PTS_K_THRES 14

typedef struct {
    Memory *mem;  // TODO - array pool index?
    AliasType alias_type;
    Memory *pt_set[PTS_K_THRES];  // Seeded NAP, ERR_MEM, NULL_MEM, ALL_MEM
    ValType val_type;
    size_t val_set[VAL_K_THRES];  // Seeded NAV, ERR_VAL, ALL_VAL
} MemStateObj;

typedef struct {
    Arena *chunks;
    Array *mems;  // Can be arena, but easier to retrieve array index
    /* Array of arrays of Memory state objects.
     * First array represents base vars in the order
     * of pool ind, followed by additional slots for
     * NAP, ERR_MEM, ALL_MEM.
     * Second array represents offset values in
     * sorted order of the constant. Reserved slots
     * for NAV, ERR_VAL, ALL_VAL. */
    Array *mem_state_objs;
} AliasState;

#endif
