// TODO: pass in context to state init helper to get pool size.
// TODO: find a better name than state init helper.
#include "liveness.h"
#include "dfa.h"

/* Placeholder identity function. */
void i(BitArray *array) {}

void free_obj(void *obj) {
    LivenessDFAObject *o = (LivenessDFAObject *) obj;
    free_bitarray(o->in);
    free_bitarray(o->out);
    free_bitarray(o->use);
    free_bitarray(o->def);
}

void free_liveness_context(void *context) {
    LivenessContext *c = (LivenessContext *) context;
    free_array(c->objs);
    free_arena(c->chunks);
    free(context);
}

void create_dfa_obj(
    Block *block,
    size_t domain,
    BitArray *use,
    BitArray *def,
    LivenessContext *context
) {
    LivenessDFAObject obj;
    obj.block = block;
    obj.in = bitarray(domain, context->chunks);
    obj.out = bitarray(domain, context->chunks);
    obj.use = use;
    obj.def = def;

    push_array(&obj, context->objs);
}

DataFlowContext *state_init_helper(
    Array *blocks, size_t domain,
    SetUse set_use, SetDef set_def
) {
    Array *objs = array(sizeof(LivenessDFAObject), free_obj); // TODO: add blocks->size
    Arena *chunks = arena(sizeof(BitChunk), 256, i);

    LivenessContext *context = malloc(sizeof(LivenessContext));
    context->objs = objs;
    context->chunks = chunks;

    /* Create data flow object for each block. */
    BitArray *tmp_use = bitarray(domain, chunks);
    BitArray *tmp_def = bitarray(domain, chunks);
    BitArray *zeros = bitarray(domain, chunks);

    for (block in blocks) {
        Array *use = bitarray(domain, chunks);
        Array *def = bitarray(domain, chunks);

        for each inst backwards {

            set_use(inst, tmp_use);
            set_def(inst, tmp_def);

            use = use U (tmp_use - tmp_def);
            def = def U tmp_def;

            reset_bitarray(tmp_use);
            reset_bitarray(tmp_def);
        }

        create_dfa_obj(block, domain, use, def, context);
    }

    free_bitarray(tmp);
    free_bitarray(zeros);

    return (DataFlowContext *) context;
}

void tf(DataFlowObject *obj) {
    LivenessDFAObject *o = (LivenessDFAObject *) obj;
    diff_bitarray(o->in, o->def, o->out);
    union_bitarray(o->use, o->out, o->out);
}

/* Liveness analysis with memory domain for DCE. */

void set_use_mem(Inst *inst, BitArray *inst_use) {
    /* For each used operand, get their memory ind through
     * operand -> memory pointer. For x = a[i],
     * get memory ind of a[i]. */
    switch (inst->type) {
        case x = *p:
            /* If p points to ALL_MEM, set all mems
             * as used (i.e., ones). */
            /* If p points to (a, ALL_VAL), set all (a, b)
             * as used. */
            /* Otherwise, set MAY or MUST pt set as used. */
            /* Also set mem of p as used. */
        case x = a[i]:
            /* If i may be ALL_VAL, set all (a, b)
             * as used. */
            /* Otherwise, set (a, val(i)) as used. */
            /* Also set mem of a, i as used. */
        default:
            /* Mark mem of RHS's as used. */
    }
}

void set_def_mem(Inst *inst, BitArray *inst_def) {
    switch (inst->type) {
        case a[i] = x:
            /* if val(i) must= b, mark (a, b) as defined. */
        case *p = x:
            /* If pt(p) must= y, mark y as defined. */
        default:
            /* Mark mem of LHS as defined. */
    }
}

DataFlowContext *state_init_mem(Array *blocks) {
    return state_init_helper(blocks, OBJ_MEM_POOL, set_use_mem, set_def_mem);
}

LivenessContext *liveness_mem(Array *blocks) {
    DataFlowContext *context = dfa_backward_default(
        blocks, i, state_init_mem, union_bitarray, tf
    );

    return (LivenessContext *) context;
}

/* Liveness analysis with vreg for live interval setup. */
// TODO