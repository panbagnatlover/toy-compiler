/* DFA framework. */

/* Separate object because data flow objects
 * are temporary and do not get reused. */
#include <stdlib.h>
#include "dfa.h"

/* Andersen */
/* domain is not known beforehand */
Allocate a fixed domain chunk to start;
global point-to map;

bool changed = true;
while (changed) {
    changed = false;

    for obj in objs {
        update val set;
        if (new value) {
            create newInt(value); // NAI, Universe included
            update bit chunk;
        }

        update pt set;
        if (new memory) {
            create newMem(value);
            update bit chunk;
        }

        update change;
    }
}

/* Flow sensitive DFA with dense bit arrays. Supports
 * both directions. Returns an array of dfa objects
 * in reverse postorder. */
Array *dfa_default(
    Array *blocks,  // in reverse postorder
    CreateVInitFunc create_v_init,
    DFAObjContextInitFunc state_init,
    MeetFunc meet,
    TransferFunc tf
) {

    DataFlowContext *context = state_init(blocks);

    /* Always the first object in object array, due to reverse postorder */
    DataFlowObject *entry_obj = (DataFlowObject *) context->objs->arr;

    create_v_init(entry_obj->out);
    bool changed = true;

    while (changed) {
        changed = false;

        for (obj in objs) {
            DataFlowObject *obj = &obj;
            Array *old_in = obj->in;
            Array *old_out = obj->out;

            DataFlowObject *prev_obj = NULL;
            for (p in obj->block->predecessors) {
                size_t ind = p->rpo_num;
                DataFlowObject *curr_obj = (DataFlowObject *) context->objs->arr + ind * context->objs->elem_size;
                meet(prev_obj->out, curr_obj->out, obj->in);
            }
            if (eq_bitarray(old_in, obj->in)) continue;

            tf(obj);
            if (!eq_bitarray(old_out, obj->out)) changed = true;
        }
    }

    return objs;
}

/* Generic DFA framework. */
State *dfa_generic(
    Array *blocks,
    DFAStateInitFunc init,
    StateFunc f
) {
    DataFlowState *state = init();

    bool changed = true;
    while (changed) {
        changed = false;

        for block in blocks {
            if (f(block)) changed = true;
        }
    }

    return state;
}
