/* End-to-end backend.
 * TODO: naming conventions of procedures. */

#include "context.h"
#include "cfg.h"
#include "aliasmap.h"
#include "optimization.h"

/* Input is a list of instructions. */
int backend(Context *c) {
    ErrCode e = NO_ERR;

    /* Build CFG. */
    Block *entry = buildCFG(c->inst_arena); // TODO: inst in arena or array?

    /* Initial simplification. */
    simplifyCFG(entry);
    Array *blocks = orderCFG(entry);

    /* SSA lifting. */
    SSALifting(blocks);

    /* Alias analysis. */
    AliasState *alias_state = alias_analysis(blocks);

    /* Mem2reg: constant propagation & reg promotion. */
    e = mem2reg(blocks, alias_state);
    if (e) goto cleanup;

    /* DCE */
    LivenessContext *l = liveness_mem(blocks);
    dce(blocks, l);
    free_liveness_context(l);

    /* Run simplification after analysis. */
    simplifyCFG(entry);
    blocks = orderCFG(entry);

    /* Instruction selection. */
    blocks = selection(blocks, c);  // TODO: dominator and memory chain

    /* TODO */
    // Liveness analysis. Create block info, inst info, etc., for scheduling and reg alloc.

    /* Pre-alloc scheduling. */
    scheduling(blocks);

    /* TODO */
    // Build live intervals

    /* Loop indexing. */
    loop_indexing(entry);

    /* Register allocation. */
    allocation(live_ranges);
    
    /* Peephole optimization. */
    // TODO

    /* Run simplification after peephole. */
    simplifyCFG(entry);
    blocks = orderCFG(entry);

    /* 2nd round scheduling. */
    scheduling(blocks);

    /* Flatten CFG. Emit final instructions. */
    flattenCFG(entry);

    return 0;

cleanup:
    return 1;
}

/* 
Assume mem2reg is run, so no *ERR, etc.
TODO: when getting mem object, set linkage with operand and variable.
for array, x = a[i] -> a[i] serve as a unity.

Step 1: use alias state obj pool size as domain.
Step 2: loop through each block, each instruction,
initialize and index bit arrays.
If all_mem MAY be used, mark all mem live (ones array).
If (a, ALL_VAL) MAY be used, mark all mem with a base live.

Note that *p = a: p cannot point to a vreg,
because if so, vreg's addr must have been taken at some point,
but that makes vreg a stack variable.
*/
