/*
 * Decision: do coalescing as a greedy pre-pass.
 * Based on the paper, greedy heuristics in a separate
 * pass does not generate materially different
 * results from the optimal integrated solution.
 * Need to implement splitting to account for the
 * drawback of register pressure as a result of coalescing.
 * 
 * Decision: split on the fly but insert moves
 * in a post-pass, along with spill code generation.
 * 
 * TODO: rewriting: when instruction is MOV a, b,
 * and b is in the stack while a not, emit LD a, b.loc
 * directly.
 * 
 * TODO: b tree
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


int priority_func(variable <> live range) {
    /* TODO: Take into account spill cost, loop
     * depth, number of uses / defs, range size, etc. */
}

occupied<register, Set<live ranges>>;  // TODO - cranelift uses a btree
precolor registers;
merge vregs that can be colaesced with no interference,
update live ranges;

push live ranges onto priority queue;

while (queue not empty) {
    get front;

    /* Try assigning incoming range to a free register. */
    if (any free register) {
        assign to free register;
        continue;
    }
    /* Find the first register where the cost
     * of spilling assigned ranges that conflict
     * with the incoming range is lower than the
     * priority of the incoming range, evict the
     * conflicting ranges, and assign the incoming
     * range. */
    else {
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

    /* TODO: makes no sense, need to update logic.
     * Profitability is estimated as the net profitability
     * of assigning split region to register while
     * spilling the other segments, against the spill
     * cost of the entire live range. We can simplify
     * the assessment by assuming a benchmark of 0
     * and calculate the profitability of the split region
     * along. Therefore, we use the following formula:
     * (the spill cost of the split region) - 
     * (the cost of extra instructions) x
     * (estimated execution weight of the outer loop). */
    /* TODO - factor in existing register pressure of
     * the split region. */
    try_hot_loop_splitting();
    try_intra_block_splitting();
    try_long_range_splitting();

    if (max profitability) {
        split at the best strategy;
        rewrite split region vreg?
        update live range somehow?
        push new ranges to queue;
        continue;
    }

    /* Spill the live range if no other allocation
     * methods apply. */
    spill;
}

int calc_spill_cost(LiveRange *lr) {
    /* Proportional to the number of uses and defs. */
    int total = 0;
    /* TODO: loop through each interval when invoked? */
    for each interval {
        total += execution_weight * (2 * uses + defs);
    }
    return total;
}

int try_hot_loop_splitting(LiveRange *lr) {
    /* Eligibility: need to cover the entire loop. */
    if (not eligible) return 0;

    /* Find all eligible splitting options.
     * Use binary search since loop info is sorted. */

    /* Estimate register pressure by (L - R) x E,
     * where L represents the number of vregs in the
     * live_in set of the loop head, R represents the
     * number of registers available, and E represents
     * the execution weight. */

    /* Calculate reg pressure before / after and add
     * delta to profitability. Update metric. */

    return max(profitability, 0), Strategy;
}

int try_inter_block_splitting(LiveRange *lr) {
    /* Eligibility: covers one or more blocks. */
    if (not eligible) return 0;

    
}

int, Array<int> try_intra_block_splitting(LiveRange *lr) {
    /* Eligibility: contains at most one block,
     * and must have at least 3 uses. */
    if (not eligible) return 0;

    int profit = 0;
    int split_pt = 0;

    /* Try to split before each use or def and
     * assess profitability. */
    /* TODO: find interval, not just a single split point */
    for each use or def after first instruction {
        int score = calc_profitability(inst);
        if (score > profit) {
            profit = score;
            split_pt = inst_ind;
        }
    }

    return profit, split_pt;
}

/* Hot loop splitting:
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
 * and insert a reverse MOV there. */
void split_around_loop(LiveRange *lr, Strategy) {

    rewrite every x in loop to x1;
    remove loop blocks from live range of x;
    create new live range for x1 and add the loop blocks;
    loop_head_info.live_in.remove(x);

    for p in loop_head->predecessors {
        if (x not live_out in p) continue;

        if (p->bl1) {
            Block *b = new Block();

            if (p->bl1 == loop_head) p->bl1 = b;
            else p->bl0 = b;

            b->bl0 = loop_head;
            b->start = MOV x1, x;
            b->end = b->start;

            /* Update live_in, live_out. */
            create block info for b;
            b_info.live_in.add(x);
            b_info.live_out.add(x1);

            /* Update live range. */
            add the block as end to live range for x;

            for all vregs in p.live_out intersect s.live_in {
                add block to vreg live range;
            }

            /* TODO - update use, def */
        }

        else {
            insert MOV x1, x right before conditional
            at the end;

            /* Update live_in, live_out. */
            p_info.live_out.add(x1);
            p_info.live_out.remove(x);

            /* Update live range. */
            for all vregs in p.live_out {
                extend interval by 1;
            }

            /* TODO - update use, def */
        }
    }

    for l in loop->exits {
        bool is_left;
        Block *s;

        if (l->bl0 out of loop) {
            is_left = true;
            s = l->bl0;
        }
        else {
            is_left = false;
            s = l->bl1;
        }

        if (x not live_in s) continue;

        if (s->predecessors.size() > 1) {
            Block *b = new Block();

            if (is_left) l->bl0 = b;
            else l->bl1 = b;

            b->bl0 = s;
            b->start = MOV x, x1;
            b->end = b->start;

            /* Update live_in, live_out. */
            create block info for b;
            b->live_in.add(x1);
            b->live_out.add(x);

            /* Update live range. */
            add the block as end to live range of x1.

            for all vregs in l.live_out intersect s.live_in {
                add the block to live range of vreg;
            }

            /* TODO - update use, def */
        }
        else {
            insert MOV x, x1 at the beginning of s;

            /* Update live_in, live_out. */
            s_info.live_in.add(x1);
            s_info.live_in.remove(x);

            /* Update live range. */
            for all vregs in s.live_in {
                extend interval by 1;
            }

            /* TODO - update use, def */
        }

        /* TODO: update interval and stuff */
    }
}

/* Inter block splitting:
 * For candidate blocks, load before first use
 * or def, and spill after last use. This means, create a new
 * spill range that consists of the block parts being spilled,
 * then for each split candidate block, create a new range. No
 * MOV insertion in this case. */
void split_around_block(LiveRange *lr) {
    /* TODO - spill code gen? */
}

/* Intra block splitting.
 * Insert MOV at the first interval boundary,
 * then, if original vreg live after the last boundary,
 * insert MOV at the last boundary.
 * No change to live in, live out sets at the block level. */
void split_within_block(LiveRange *lr) {
    insert MOV x1, x at first splitting point;
    if x live at second splitting point, insert MOV x, x1
    at second splitting point;

    for each instruction in between:
        allocate to new index;
        replace x with x1;
    
    split live range of x for that block:
        end at first splitting point;
        if second splitting point, start after the second MOV;
    
    /* Update live intervals */
    Get all vregs live at original first splitting point
    (before MOV);
    If an interval ends before original second splitting
    point, adjust by increment ending by 1;
    If an interval live at original second splitting point,
    adjust by increment ending by 2;

    Get all vregs live after original first splitting point
    and before or at second splitting point, adjust by
    increment ending by 1;

    /* TODO: update use defs */
}

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

	evicted = Set();

	for each RHS vreg
		if it is spilled
            if it already lives in a free register, i.e., assignment[vreg] = register
                keep assignment[vreg], do nothing;
            else if MOV instruction
                change instruction to LD;
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
    emit instruction;

	if LHS variable is spilled
		Emit ST assignment[vreg], vreg.location;
        assignment[vreg] = spilled;

	for each evicted vreg
		Emit LD range_assignment[vreg, slot], vreg.location
        assignment[vreg] = range_assignment[vreg, slot];
}
