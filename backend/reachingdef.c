// TODO: pass in context to state init helper to get pool size.
// TODO: find a better name than state init helper.
/* Reaching definitions. */
#include "reachingdef.h"
#include "dfa.h"

/* Placeholder identity function. */
void i(BitArray *array) {}

void free_obj(void *obj) {
    ReachingDefDFAObject *o = (ReachingDefDFAObject *) obj;
    free_bitarray(o->in);
    free_bitarray(o->out);
    free_bitarray(o->gen);
    free_bitarray(o->kill);
}

void free_reaching_def_context(void *context) {
    ReachingDefContext *c = (ReachingDefContext *) context;
    free_array(c->objs);
    free_arena(c->chunks);
    free_array(c->defs);
    free(context);
}

void create_dfa_obj(
    Block *block,
    size_t domain,
    BitArray *gen,
    BitArray *kill,
    ReachingDefContext *context
) {
    size_t ind = block->rpo_num;

    ReachingDefDFAObject obj;
    obj.block = block;
    obj.in = bitarray(domain, context->chunks);
    obj.out = bitarray(domain, context->chunks);
    obj.gen = gen;
    obj.kill = kill;

    push_array(&obj, context->objs);
}

DataFlowContext *state_init_helper(Array *blocks, size_t domain, SetKillSet set_func, GetKillSet get_func) {
    Array *objs = array(sizeof(ReachingDefDFAObject), free_obj); // TODO: add blocks->size
    Arena *chunks = arena(sizeof(BitChunk), 256, i);
    Array *defs = array(sizeof(BitArray *), free_bitarray); // TODO: add var_pool_size

    ReachingDefContext *context = malloc(sizeof(ReachingDefContext));
    context->objs = objs;
    context->defs = defs;
    context->chunks = chunks;

    /* Map each var's definition into a bitarray. */
    for (block in blocks) {
        for (each inst) set_func(inst, context);
    }

    /* Create data flow object for each block. */
    BitArray *tmp = bitarray(domain, chunks);
    BitArray *zeros = bitarray(domain, chunks);

    for (block in blocks) {
        Array *gen = bitarray(domain, chunks);
        Array *kill = bitarray(domain, chunks);

        for each inst {
            set_bitarray(inst->ind, true, tmp);

            BitArray *defs = get_func(inst, c);

            /* gen = gen + inst bit - kill.
             * kill = kill + defs. */
            union_bitarray(gen, tmp, gen);
            diff_bitarray(gen, kill, gen);
            union_bitarray(kill, defs, kill);

            reset_bitarray(tmp);
        }

        create_dfa_obj(block, domain, gen, kill, context);
    }

    free_bitarray(tmp);
    free_bitarray(zeros);

    return (DataFlowContext *) context;
}

void tf(DataFlowObject *obj) {
    ReachingDefDFAObject *o = (ReachingDefDFAObject *) obj;
    diff_bitarray(o->in, o->kill, o->out);
    union_bitarray(o->gen, o->out, o->out);
}

/* Implementation for reaching def for ssalifting. */

/* For ssalifting: p = &x; *p = a; does not kill x = y. */
void set_kill_set_ssa(Inst *inst, ReachingDefContext *c) {
    switch (inst->type) {
        case BLABLA:  // If inst has a clear lhs
            BitArray *a = c->defs[inst->lhs->var];
            if (!a) {
                a = bitarray(domain, chunks);
                defs[inst->lhs->var] = a;
            }
            set_bitarray(inst->ind, true, a);
        case *p = x:
            return;
        ...
    }
}

BitArray *get_kill_set_ssa(Inst *inst, ReachingDefContext *c) {
    if (inst is def) return defs[inst->lhs->var];
    else return zeros;
}

DataFlowContext *state_init_ssa(Array *blocks) {
    return state_init_helper(blocks, INST_POOL_SIZE, set_kill_set_ssa, get_kill_set_ssa);
}

ReachingDefContext *reaching_def_ssa(Array *blocks) {
    DataFlowContext *context = dfa_forward_default(
        blocks, i, state_init_ssa, union_bitarray, tf
    );
    return (ReachingDefContext *) context;
}

/* Implementation for reaching def for use def chain setup. */

void set_kill_set_vreg(Inst *, ReachingDefContext *c) {

}

BitArray *get_kill_set_vreg(Inst *, ReachingDefContext *c) {

}
// TODO
