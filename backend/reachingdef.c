/* Reaching definitions. */
#include "reachingdef.h"
#include "dfa.h"

/* Placeholder identity function. */
void i(BitArray *array) {}

void free_obj(void *obj) {
    ReachingDefDFAObject *o = (ReachingDefDFAObject *) o;
    free_bitarray(o->in);
    free_bitarray(o->out);
    free_bitarray(o->gen);
    free_bitarray(o->kill);
}

void free_reaching_def_context(ReachingDefContext *context) {
    free_array(context->objs);
    free_arena(context->chunks);
    free_array(context->defs);
    free(context);
}

void create_dfa_obj(
    Block *block,
    size_t domain,
    bool init_val,
    BitArray *gen,
    BitArray *kill,
    ReachingDefContext *context
) {
    size_t ind = block->rpo_num;
    void *loc = (char *) context->objs->arr + ind * context->objs->elem_size;

    ReachingDefDFAObject obj;
    obj.block = block;
    obj.in = bitarray(domain, context->chunks);
    obj.out = bitarray(domain, context->chunks);
    obj.gen = gen;
    obj.kill = kill;

    if (init_val) {
        ones_bitarray(obj.in);
        ones_bitarray(obj.out);
    }

    memcpy(loc, obj, sizeof(ReachingDefDFAObject));
}

DataFlowContext *state_init(Array *blocks) {
    Array *objs = array(sizeof(ReachingDefDFAObject), free_obj); // TODO: add blocks->size
    Arena *chunks = arena(sizeof(BitChunk), 256, i);
    Array *defs = array(sizeof(BitArray *), free_bitarray); // TODO: add var_pool_size

    ReachingDefContext *context = malloc(sizeof(ReachingDefContext));
    context->objs = objs;
    context->defs = defs;
    context->chunks = chunks;

    size_t domain = inst_pool_size;

    /* Map each var's definition into a bitarray. */
    for (block in blocks) {
        for each inst {
            if (inst is def) {
                BitArray *a = defs[inst->lhs->var];
                if (!a) {
                    a = bitarray(domain, chunks);
                    defs[inst->lhs->var] = a;
                }
                set_bitarray(inst->ind, true, a);
            }
        }
    }

    /* Create data flow object for each block. */
    BitArray *tmp = bitarray(domain, chunks);
    BitArray *zeros = bitarray(domain, chunks);

    for (block in blocks) {
        Array *gen = bitarray(domain, chunks);
        Array *kill = bitarray(domain, chunks);

        for each inst {
            set_bitarray(inst->ind, true, tmp);

            BitArray *def;
            if (inst is def) def = defs[inst->lhs->var];
            else def = zeros;

            /* gen = gen + inst bit - kill.
             * kill = kill + (defs - inst bit). */
            union_bitarray(gen, tmp, gen);
            diff_bitarray(gen, kill, gen);
            diff_bitarray(def, tmp, tmp);
            union_bitarray(kill, tmp, kill);

            reset_bitarray(tmp);
        }

        create_dfa_obj(block, domain, false, gen, kill, context);
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

ReachingDefContext *reaching_def(Array *blocks) {
    DataFlowContext *context = dfa_default(
        blocks, i, state_init, union_bitarray, tf
    );

    return (ReachingDefContext *) context;
}
