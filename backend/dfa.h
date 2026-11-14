/* TODO - needed? DFA framework. */
#include "block.h"
#include "bitarray.h"

typedef struct {
    BitArray *in;
    BitArray *out;
    Block *block;
} DataFlowObject;

typedef struct {
    Array *objs;  // Data flow objects
    Arena *chunks;  // Bitarray chunks
} DataFlowContext;

typedef DataFlowContext *(*DFAObjContextInitFunc)(Array *);  // Input is array of blocks; output is state object
typedef void (*CreateVInitFunc) (BitArray *);
typedef void (*MeetFunc)(BitArray *, BitArray *, BitArray *);
typedef void (*TransferFunc)(DataFlowObject *);

typedef void *(*DFAStateInitFunc)(void);
typedef bool (*StateFunc)(Block *block, void *state);  // return true if state changed

Array *dfa_default(
    Array *blocks,  // in reverse postorder
    CreateVInitFunc create_v_init,
    DFAObjContextInitFunc state_init,
    MeetFunc meet,
    TransferFunc tf
);
