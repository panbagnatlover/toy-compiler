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

void scheduling(Array *blocks, bool pre_alloc, LivenessDFAObject *objs);

#endif
