#ifndef H_SCHEDULING
#define H_SCHEDULING

#include "array.h"
#include "dict.h"
#include "liveness.h"

/* Lookback window for list scheduling
 * in a circular buffer. */
#define RESOURCE_LOOKBACK 10

/* Size of the resource table. */
#define BUFFER_SIZE MAX_CYCLES + RESOURCE_LOOKBACK

/* Threshold at which reg-aware scheduling
 * heuristic is invoked. */
#define REG_THRESHOLD 30  // TODO: this is bs value rn

typedef struct {
    Inst *inst;
    int depth;  // Longest latency path from entry to curr instruction
    int outdegree;  // Current number of unscheduled successors
    Dict pred_latencies;  // Predecessor latencies
    Dict succ_latencies;  // Successor latencies
    uint8_t res_demand[NUM_RESOURCE_TYPE];  // Total units demanded by resource
    size_t schedule;  // Relative time cycle of the scheduled instruction in a block
} InstInfo;

typedef struct {
    Array inst_info_arr;  // fixed size block->insts.size

    /* Remaining resource demand. Sum of all units needed
    * for a resource from all unscheduled instructions. */
    int rrd[NUM_RESOURCE_TYPE];

    /* Resource table. Implemented as a circular buffer. */
    int rt[BUFFER_SIZE][NUM_RESOURCE_TYPE];

    /* Mapping between instruction->ind to local
     * instruction order. This array is never reset
     * across blocks. */
    Array inst_order;
} BlockInfo;

void scheduling(Array *blocks, bool pre_alloc, LivenessDFAObject *objs);

#endif
