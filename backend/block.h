/* Basic block data structure and functions. */
/* TODO - use one struct */

#ifndef H_BLOCK
#define H_BLOCK

#include "dll.h"

typedef struct {
    enum {ENTRY, BODY, EXIT} type;
    size_t ind;         // Index in pool. TODO: needed?
    size_t rpo_num;     // Order in reverse postorder
    DLL insts;
    Array predecessors;  // Array of Block * objects
    Block *bl0;
    Block *bl1;
} Block;

#endif
