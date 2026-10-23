#ifndef H_LIVE_RANGE
#define H_LIVE_RANGE

#include <stdlib.h>

typedef struct Interval {
    size_t start;
    size_t end;
};

typedef struct LiveRange {
    Variable *var;
    Array<Interval> range;

    enum {
        INTRA_BLOCK,
        HOT_LOOP,
        LONG_RANGE
    } split_type;

    int assigned_reg;
    int spill_cost;
    int loop_depth;
    ...
};

#endif
