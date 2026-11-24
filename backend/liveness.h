#ifndef H_LIVENESS
#define H_LIVENESS

#include "array.h"
#include "arena.h"
#include "inst.h"

typedef void (*SetUse)(Inst *, BitArray *inst_use);
typedef void (*SetDef)(Inst *, BitArray *inst_def);

typedef struct {
    BitArray *in;  // TODO: not pointer
    BitArray *out;  // TODO: not pointer
    Block *block;
    BitArray *use;  // TODO: not pointer
    BitArray *def;  // TODO: not pointer
} LivenessDFAObject;

typedef struct {
    Array *objs;  // Array of dfa objects, in rp order of blocks
    Arena *chunks;  // Arena of bit chunks
} LivenessContext;

/* TODO: move to application side. Add state init helper. */
LivenessContext *liveness_mem(Array *blocks);
LivenessContext *liveness_vreg(Array *blocks);
void free_liveness_context(void *context);

#endif
