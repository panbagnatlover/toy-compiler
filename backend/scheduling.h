#ifndef H_SCHEDULING
#define H_SCHEDULING

#include <stdlib.h>

typedef struct {
    int inst_ind;
    int depth;
    int rp_diff;
    int resource_demand[RESOURCES];
    Array<size_t> predecessors;  // TODO: fixed size of block inst count?
    Array<size_t> p_latencies;
    Array<size_t> successors;
    Array<size_t> s_latencies;
} InstInfo;

#endif