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
void mem2reg(Array *blocks, AliasState *alias_state) {
    for (block in blocks) {
        for (each inst) {
            if (lhs) {
                MemStateObj *obj = get_mem_state(lhs->var, 0);
                if (obj->val_type == MUST_VAL) {
                    change inst to lhs = obj->val_set[3];
                }
            }
            for (each used vreg) {
                MemStateObj *obj = get_mem_state(vreg->var, 0);
                if (obj->val_type == MUST_VAL) {
                    change vreg to obj->val_set[3] constant;
                }
            }

            if (*p = y and pt(p) MUST= {mem(x, 0)}) -> x = y;
            if (x = *p and pt(p) MUST= {mem(y, 0)}) -> x = y;
        }
    }
}
