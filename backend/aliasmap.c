/* Alias analysis.
 * Since we cap the size of point-to and val sets,
 * no need to use bitarrays. Fixed arrays suffice.
 */

#include "arena.h"
#include "dfa.h"
#include "memory.h"

/* If val(x) can take on more than 10 constants,
 * set val(x) = ALL_VAL. Same for pt(x). */
#define VAL_K_THRES 13
#define PTS_K_THRES 14

typedef struct {
    Memory *mem;  // TODO - array pool index?
    AliasType alias_type;
    Memory *pt_set[PTS_K_THRES];  // Seeded NAP, ERR_MEM, NULL_MEM, ALL_MEM
    ValType val_type;
    size_t val_set[VAL_K_THRES];  // Seeded NAV, ERR_VAL, ALL_VAL
} MemStateObj;

typedef struct {
    Arena *chunks;
    Array *mems;  // Can be arena, but easier to retrieve array index
    /* Array of arrays of Memory state objects.
     * First array represents base vars in the order
     * of pool ind, followed by additional slots for
     * NAP, ERR_MEM, ALL_MEM.
     * Second array represents offset values in
     * sorted order of the constant. Reserved slots
     * for NAV, ERR_VAL, ALL_VAL. */
    Array *mem_state_objs;
} AliasState;

void i(void *) {};

MemStateObj *get_mem_state(Var *var, Val offset, AliasState *state) {
    Array *objs = state->mem_state_objs[var->ind];
    // TODO: optimize
    for (obj in objs) {
        if (obj.mem->type == MEM && obj.mem->val == offset) return &obj;
    }
    return NULL;
}

void *init_state(void) {
    Arena *chunks = arena(sizeof(BitChunk), 256, i);
    Arena *mems = arena(sizeof(Memory), 256, i);
    Array *mem_state_objs = array(sizeof(Array *), free_array);
}

bool state_tf(void *b, void *s) {
    Block *block = (Block *) b;
    AliasState *state = (AliasState *) s;

    bool changed = false;

    for (inst in block->insts) {
        switch (inst->type) {
            case a = b + c:
                MemStateObj *c_mem_obj = get_mem_state(c, 0, state);
                Mem *c_mem = c_mem_obj->mem;
                Array *c_vals = c_mem_obj->val_set;
                Memory **c_pts = c_mem_obj->pt_set;

                // ... same for a, b

                if (c_mem_obj->alias_type != NO_ALIAS) {
                    for val in b_vals {
                        if (val == ERR) then blabla
                        if (val == NAV) then blabla
                        if (val == ALL) then blabla
                        set or find memory at mem(x, val) for x in pt(c)
                        add mem(x, val) to c_mem_obj->pt_set;
                    }
                }
                if (b_mem_obj->alias_type != NO_ALIAS) {
                    blabla;
                }
            case *p = a:
                ...
            ...
        }
    }

    return changed;
}

AliasState *alias_analysis(Array *blocks) {
    void *state = dfa_generic(blocks, init_state, state_tf);
    return (AliasState *) state;
}
