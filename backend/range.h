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

typedef struct {
    size_t ind;
    Block *block;
    Slot *prev;
    Slot *next;
    Inst *inst_after;  // The instruction immediately after the slot.
} Slot;

typedef struct {
    // size_t start_ind;
    // size_t end_ind;
    size_t block_ind;
    Slot *start;
    Slot *end;
    /* Decision: no need to maintain live_in or live_out
     * after live intervals are built for the first time.
     * Can just query on slot. */
    Set<Vreg *> live_in;
    Set<Vreg *> live_out;
} BlockInfo;  // Same index in pool as Block

typedef struct {
    Loop *loop;
    Array<Interval> intervals;
    int execution_weight;
} LoopInfo;

typedef struct {
    size_t inst_ind;
    Slot *slot_before;
} InstInfo;

#endif
