#include "aliasmap.h"
#include "liveness.h"

/* Local copy propagation. */
void local_cp(Block *block) {
    x = y
    y = z
    
}

/* Dead code elimination. */
void dce(Array *blocks, LivenessContext *liveness_context) {
    BitArray *tmp_use = bitarray();
    BitArray *tmp_def = bitarray();

    for (block in blocks) {
        LivenessDFAObject *obj = liveness_context->objs[block->ind];
        BitArray *live = obj->out;

        for (each inst backwards) {

            switch (inst->type) {
                case a[i] = x:
                    if (pt(a[i]) MUST= something) {
                        get ind of pointed to mem;
                        if (!bool_bitarray(ind, live)) {
                            remove inst;
                            continue;
                        }
                    }
                    get ind of (a, 0);
                    if (!bool_bitarray(ind, live)) {
                        remove inst;
                        continue;
                    }
                    break;
    
                case *p = x:
                    if (pt(p, 0) MUST= something) {
                        get ind of pointed to mem;
                        if (!bool_bitarray(ind, live)) {
                            remove inst;
                            continue;
                        }
                    }
                    break;
                default:  // x = something
                    /* If LHS is not live after definition,
                     * remove the instruction. */
                    get ind of (x, 0);
                    if (!bool_bitarray(ind, live)) {
                        remove inst;
                        continue;
                    }
            }

            set_def_mem(inst, tmp_def);
            set_use_mem(inst, tmp_use);
            live = tmp_use U (live - tmp_def);

            reset_bitarray(tmp_def);
            reset_bitarray(tmp_use);
        }
    }

    // TODO: free tmp bitarrays? Or the chunk arena?
}

/* Constant propagation + register promotion. */
ErrCode mem2reg(Array *blocks, AliasState *alias_state) {
    for (block in blocks) {
        for (each inst) {
            /* If LHS MUST= a value, change the instruction
             * to a copy instruction and assign LHS that value.
             * If the resulting instruction yields semantic error,
             * e.g., *1 = a, raise error. */
            if (lhs) {
                MemStateObj *obj = get_mem_state(lhs->var, 0);
                if (obj->val_set.val_type == MUST_VAL) {
                    change inst to lhs = val;
                }
            }
            /* If RHS MUST= a value, replace RHS with that value.
             * If the resulting instruction yields semantic error,
             * e.g., y = &1, raise error. */
            for (each used var operand) {
                MemStateObj *obj = get_mem_state(operand.var, 0);
                if (obj->val_set.val_type == MUST_VAL) {
                    change var operand to constant;
                }
            }

            /* if instruction is of the form *p = y,
             * and pt(p) MUST= {x}, then rewrite the
             * instruction to x = y. */
            if (inst->type == INST_LHS_DEREF) {
                MemStateObj *obj = get_mem_state(lhs.var, 0);
                if (obj->pt_set.pt_type != MUST_MEM) continue;
                MemStateObj *n = first item in pt_set;
                if (n->mem.type == ERR_MEM) {
                    raise error("attempting to dereference a variable of type {lhs.var->type}.");
                    continue; // TODO: failure mode
                }
                else if (n->mem.offset.value = 0) {
                    change inst to copy inst and replace lhs with n->mem.base;
                }
            }
            /* If instruction is of the form x = *p,
             * and pt(p) MUST= {y}, then rewrite the
             * instruction to x = y. */
            else if (inst->type == INST_RHS_DEREF) {
                MemStateObj *obj = get_mem_state(rhs.var, 0);
                if (obj->pt_set.pt_type != MUST_MEM) continue;
                MemStateObj *n = first item in pt_set;
                if (n->mem.type == ERR_MEM) {
                    raise error("attempting to dereference a variable of type {lhs.var->type}.");
                    continue; // TODO: failure mode
                }
                else if (n->mem.offset.value = 0) {
                    change inst to copy inst and replace rhs with n->mem.base;
                }
            }
        }
    }

    return ErrCode;
}
