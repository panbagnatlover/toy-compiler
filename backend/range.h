#ifndef H_RANGE
#define H_RANGE

#include <stdlib.h>

/* Start, end are slots rather than instruction indices.
 * Therefore, for each interval, slot size = instruction size + 1. */
typedef struct {
    size_t block_ind;
    Slot *start;
    Slot *end;
    LiveRange *range;
    Array<int> uses;
    Array<int> defs;
} Interval;

// TODO: need a mapping from vreg to its live range obj
// TODO: put spill weight, var at interval level?
// TODO: occupied is a mapping from reg to intervals,
// allow vreg mapping to support same query for .find etc.
typedef struct {
    Variable *var; /* TODO: may not need here? */
    Array<Interval> intervals;
    Reg assigned_reg;  // 0 = spill
    int spill_weight;  // SIZE_MAX = unset
} LiveRange;

/* Slot is a DLL-backed type. */
typedef struct {
    Slot *prev;
    Slot *next;

    /* Local ind inside a block. */
    size_t ind;
    Block *block;  // TODO: needed?
    Inst *inst;  // The instruction immediately after the slot.
} Slot;

#endif
