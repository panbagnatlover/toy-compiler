#ifndef H_RANGE
#define H_RANGE

#include <stdlib.h>

typedef struct {
    size_t start;
    size_t end;
} Interval;

typedef struct {
    Variable *var;
    Array<Interval> range;
    Array<size_t> defs;
    Array<size_t> uses;

    int assigned_reg;  // 0 = spill
    int spill_cost;
    int loop_depth;
} LiveRange;

/* Needed to record live_in, live_out, etc. */
typedef struct {
    // size_t start_ind;
    // size_t end_ind;
    size_t block_ind;
    Array<int> inst_array;
    Set<Vreg *> live_in;
    Set<Vreg *> live_out;
} BlockInfo;

/* TODO - needed? */
typedef struct {
    // size_t start_ind;
    // size_t end_ind;
    Loop *loop;
} LoopInfo;

#endif
