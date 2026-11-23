#ifndef H_REACHINGDEF
#define H_REACHINGDEF

#include "array.h"
#include "arena.h"
#include "block.h"

typedef struct {
    BitArray *in;  // TODO: not pointer
    BitArray *out;  // TODO: not pointer
    Block *block;
    BitArray *gen;  // TODO: not pointer
    BitArray *kill;  // TODO: not pointer
} ReachingDefDFAObject;

typedef struct {
    Array *objs;  // Array of dfa objects, in rp order of blocks
    Arena *chunks;  // Arena of bit chunks
    Array *defs;  // Mapping of var ind -> bitarray
} ReachingDefContext;

ReachingDefContext *reaching_def(Array *blocks);
void free_reaching_def_context(void *context);

#endif
