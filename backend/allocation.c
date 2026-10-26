/*
 * Decision: do coalescing as a greedy pre-pass.
 * Based on the paper, greedy heuristics in a separate
 * pass does not generate materially different
 * results from the optimal integrated solution.
 * Implement splitting however, to account for the
 * drawback of register pressure as a result of coalescing.
 * 
 * Decision: split on the fly but insert moves
 * in a post-pass, along with spill code generation.
 */

/* Example: be careful with MOV insertion at
 * arbitrary program points */
x = 1
y = x
if (cond) goto L1
x = 2
y = x
L1: z = x

x = 1
y = x
x2 = x
if (cond) goto L1
x2 = 2
y = x2
L1: z = x2

/* Example: Hot loop splitting - single loop */
x = 1
y = x
loop start: x = x + 1
if (cond) goto L1
z = x * 2
L1: y = x * 2
if (cond) goto loop start
z = x

x = 1
y = x
x2 = x
loop start: x2 = x2 + 1
if (cond) goto L1
z = x2 * 2
L1: y = x2 * 2
if (cond) goto loop start
x = x2
z = x

/* Example: hot loop splitting - nested loop */
x = 1
y = x
loop 1: x = x + 1
if (cond) goto loop 1
x = x * 2
y = x - 1
if (cond) goto loop 1
...

x = 1
y = x
x2 = x
loop 1: x2 = x2 + 1
if (cond) goto loop 1
x = x2
x = x * 2
y = x - 1
if (cond) goto L3
...
L3: x2 = x
goto loop 1

/* Example: long range splitting */
x = 2
if (cond) goto L1
x = 1
y = x
... (no x)
if (cond) goto L2
L1: z = x
x = 4
L2: y = x

x = 2
if (cond) goto L1
x = 1
y = x
... (no x)
x2 = x
if (cond) goto L2
L1: z = x2
x2 = 4
L2: y = x2

/* Example: intra block splitting */
x = 1
y = x
z = x + y
x = 2

x = 1
y = x
x2 = x
z = x2 + y

if (cond) goto L1
x = 1
y = x
x2 = x
z = x2 + y
x2 = 2
x = x2
L1: z = x

/* Intra block splitting:
 * If interval within a block,
 * insert MOV at the first interval boundary,
 * then, if original vreg live after the last boundary,
 * insert MOV at the last boundary.
 * 
 * Hot loop splitting:
 * If interval at loop boundary, for every
 * predecessor out of the loop that has multiple
 * successors (i.e., conditional branch),
 * create a new block with MOV and goto loop head.
 * For every predecessor out of the loop that has
 * a single successor, insert MOV before
 * each predecessor's jump instruction. For every exit
 * block (i.e., block in the loop that can branch out
 * of the loop), note that each block must be a conditional.
 * If the original vreg is live in the exit branch, add a block
 * between the exit block and the successor out of the loop,
 * and insert a reverse MOV there.
 * Note that splitting multiple loops headed by the same
 * instruction is not possible.
 * 
 * Long range splitting:
 * If register pressure in the short term is high, 
 * there's a far far away next use or redef, no holes,
 * the latest dominating block before the next use or redef
 * reverse dominates all uses and redefs in the short term,
 * split at the latest dominating block before the next use
 * or redef and spill the first segment. Insert 
 * LDR at the latest dominating block of all the next uses
 * and redefs.
 * 
 * If the above but holes, split at holes.
 * */

int priority_func(variable <> live range) {
    /* TODO: Take into account spill cost, loop
     * depth, number of uses / defs, range size, etc. */
}

occupied<register, Set<live ranges>>;  // TODO - cranelift uses a btree
precolor registers;
map of vregs that can be colaesced with no interference;

push all variable <> live ranges onto priority queue;

while (queue not empty) {
    get front;

    /* Try assigning incoming range to a free register. */
    if (any free register) {
        assign to free register;
        continue;
    }
    else {
        /* Find the first register where the cost
         * of spilling assigned ranges that conflict
         * with the incoming range is lower than the
         * priority of the incoming range, evict the
         * conflicting ranges, and assign the incoming
         * range. */
        for (each register) {
            find conflicting ranges;
            if (cost of spilling the conflicting ranges < spill weight of front) {
                evict the conflicting ranges, push to queue;
                assign front to register;
                break;
            }
        }
        if (assigned) continue;
    }

    /* Try splitting the incoming range
     * for better assignment opportunities, and
     * add the new ranges to queue. Note that
     * this cannot result in infinite recursion
     * since the split ranges can only be smaller,
     * and at some point, the ranges cannot be split
     * further. */
    int split_profit_hot_loop;
    int split_profit_intra_block;
    int split_profit_long_range;

    /* Profitability is estimated as the net profitability
     * of assigning split region to register while
     * spilling the other segments, against the spill
     * cost of the entire live range. We can simplify
     * the assessment by assuming a benchmark of 0
     * and calculate the profitability of the split region
     * along. Therefore, we use the following formula:
     * (the spill cost of the split region) x
     * (estimated execution weight of the split region) - 
     * (the cost of MOV insertions) x
     * (estimated execution weight of the outer loop). */
    /* TODO - factor in existing register pressure of
     * the split region. */
    /* TODO - need to rewrite vreg */
    try_hot_loop_splitting();
    try_intra_block_splitting();
    try_long_range_splitting();

    if (max profitability) {
        split at the best strategy;
        if long range, split the current, push the next segment to queue;
        else, push new and changed ranges to queue;
        continue;
    }

    /* Spill the live range if no other allocation
     * methods apply. */
    spill;
}

int, Strategy try_hot_loop_splitting(LiveRange *lr) {
    if (not eligible) return 0;

    /* Find all eligible splitting options */

    return max(profitability, 0), Strategy;
}

void split_by_loop_boundary(LiveRange *lr, Strategy) {
    /* TODO */
    /* Insert MOVs */
    /* Update ranges (intervals, uses, defs, spill cost, etc.) */
    /* Push back to worklist */
}

/* TODO - hot loop move insertion
 * assign the loop of the predecessor when inserting
 * a pre-header; assign the loop of the successor when
 * inserting a post-header. */

/* Spill rewrite + move insertion */
assignment = Dict(); // Current assignment. vreg -> register

LiveRange *get_live_range(VReg *vreg) {
    /* TODO */
}

for (each instruction, slot) {
    if (inserted by move insertion, so no slot) {
        update assignment;
        continue;
    }

    /* TODO: Move insertion */
    for each vreg {
        LiveRange *live_range = get_live_range(vreg);
        if (not at split juncture) continue;
        switch (live_range->split_type) {
            case INTRA_BLOCK:
                break;
            case HOT_LOOP;
                break;
            case LONG_RANGE;
                break;
            default:
        }
    }

	evicted = Set();

	for each RHS vreg
		if it is spilled
            if it already lives in a free register, i.e., assignment[vreg] = register
                keep assignment[vreg], do nothing;
			else if there exists a free register at slot
                set assignment[vreg] = register;
				Emit LD register, vreg.location;
			else
                find a register not holding other RHS vregs and evict the current held vreg;
				Emit ST register, evicted vreg;
				Emit LD register, vreg;
                assignment[vreg] = register;
                assignment[evicted vreg] = evicted;

	if LHS variable is spilled
		if there exists a free register
            set assignment[vreg] = register
		else
            find a register not holding RHS variables, evict the variable held
            Emit ST register, evicted vreg;
            assignment[vreg] = register;
            assignment[evicted vreg] = evicted;

	replace vreg with assignment[vreg];

	if LHS variable is spilled
		Emit ST assignment[vreg], vreg.location;
        assignment[vreg] = spilled;

	for each evicted vreg
		Emit LD range_assignment[vreg, slot], vreg.location
        assignment[vreg] = range_assignment[vreg, slot];
}
