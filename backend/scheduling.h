#ifndef H_SCHEDULING
#define H_SCHEDULING

#include <stdlib.h>

typedef struct {
    int inst_ind;
    int height;
    int rp_diff;
    int resource_demand[RESOURCES];
    int num_successors;
} InstInfo;

#endif