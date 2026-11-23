#include "aliasmap.h"

/* Local copy propagation. */
void local_cp(Block *block) {
    x = y
    y = z
    
}

/* Local dead code elimination. */
void local_dce(Block *block, BitArray *live_out) {
    
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
