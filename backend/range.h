#ifndef H_RANGE
#define H_RANGE

#include <stdlib.h>

typedef struct {
    size_t block_ind;
    size_t start;
    size_t end;
    Array<int> uses;
    Array<int> defs;
} Interval;

typedef struct {
    Variable *var;
    Array<Interval> intervals;
    int assigned_reg;  // 0 = spill
} LiveRange;

typedef struct {
    // size_t start_ind;
    // size_t end_ind;
    size_t block_ind;
    Array<int> inst_array;  // TODO - needed? Or just count
    Set<Vreg *> live_in;
    Set<Vreg *> live_out;
} BlockInfo;  // Same index in pool as Block

typedef struct {
    Loop *loop;
    Array<Interval> intervals;
} LoopInfo;

#endif
