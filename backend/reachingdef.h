#ifndef H_REACHINGDEF
#define H_REACHINGDEF

#include "array.h"
#include "arena.h"
#include "inst.h"

typedef void (*SetKillSet)(Inst *inst, Array *defs);
typedef BitArray *(*GetKillSet)(Inst *inst, Array *defs);

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

// TODO: move this to application specific,
// only bring state init helper, tf, etc.
ReachingDefContext *reaching_def_ssa(Array *blocks);
ReachingDefContext *reaching_def_vreg(Array *blocks);
void free_reaching_def_context(void *context);

#endif
