/* End-to-end backend.
 * TODO: naming conventions of procedures. */

#include "inst.h"
#include "cfg.h"
#include "aliasmap.h"
#include "optimization.h"

/* Input is a list of instructions. */
int backend(Inst *insts) {
    ErrCode e = NO_ERR;

    /* Build CFG. */
    Block *entry = buildCFG(insts);

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
    // TODO;

    /* Run simplification after analysis. */
    simplifyCFG(entry);
    blocks = orderCFG(entry);

    /* TODO */
    // Tag each variable memory vs vreg;

    /* Instruction selection. */
    selection(blocks, rules?);

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
