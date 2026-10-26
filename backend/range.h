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

typedef struct {
    size_t start_ind;
    size_t end_ind;
    MBlock *block;
} BlockInfo;

typedef struct {
    size_t start_ind;
    size_t end_ind;
    Loop *loop;
} LoopInfo;

#endif
