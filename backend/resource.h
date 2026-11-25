#ifndef H_RESOURCE
#define H_RESOURCE

#define NUM_RESOURCE_TYPE 3

#include <stdlib.h>

typedef enum {
    RESOURCE_ALU = 0,
    RESOURCE_STR,
    RESOURCE_LDR
} Resource;

uint8_t available_resources[NUM_RESOURCE_TYPE] = {
    2, 2, 2
};

#endif
