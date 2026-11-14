/* Basic block data structure and functions. */
/* TODO - use one struct */

#ifndef H_BLOCK
#define H_BLOCK

#include "dll.h"

typedef struct Inst;

typedef struct {
    enum {ENTRY, BODY, EXIT} type;
    size_t ind;         // Index in pool
    size_t rpo_num;     // Order in reverse postorder

    DLL *instructions;  // TODO: dll implementation

    Set<struct Block *> predecessors;
    struct Block *bl0;
    struct Block *bl1;
} Block;

// typedef struct MBlock {
//     enum {ENTRY, BODY, EXIT} type;
//     struct MInst *head;
//     struct MInst *tail;
//     Set<struct MBlock *> predecessors;
//     struct MBlock *bl0;
//     struct MBlock *bl1;
//     BitSet defs;
//     BitSet uses;
// }

#endif
