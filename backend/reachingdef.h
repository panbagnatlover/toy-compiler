#ifndef H_REACHINGDEF
#define H_REACHINGDEF

#include "array.h"
#include "arena.h"
#include "block.h"

typedef struct {
    BitArray *in;
    BitArray *out;
    Block *block;
    BitArray *gen;
    BitArray *kill;
} ReachingDefDFAObject;

typedef struct {
    Array *objs;  // Array of dfa objects
    Arena *chunks;  // Arena of bit chunks
    Array *defs;  // Mapping of var ind -> bitarray
} ReachingDefContext;

void free_reaching_def_context(void *);

#endif
