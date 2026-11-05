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


int priority_func(LiveRange *lr) {
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
    strategy p1, p2, p3;
    strategy max_profit;

    p1 = try_hot_loop_splitting(front);
    p2 = try_intra_block_splitting(front);
    p3 = try_long_range_splitting(front);
    max_st = p1.profit > p2.profit ? p1 : p2;
    max_st = max_st.profit > p3.profit ? max_st : p3;

    if (max_st.profit) {
        switch (max_st.strategy) {
            case HOT_LOOP:
                ranges = split_around_loop(front);
                push ranges to queue;
                break;
            case INTER_BLOCK:
                ranges = split_around_block(front);
                spill background range;
                push foreground ranges to queue;
                break;
            case INTRA_BLOCK:
                ranges = split_within_block(front);
                break;
            default:
        }

        push new ranges to queue;
        continue;
    }

    /* Spill the live range if no other allocation
     * methods apply. */
    spill;
}

/* Estimate split profitability as 
 * abs(spill weight of the split region minus minus
 * estimated spill cost due to reg pressure), minus
 * cost of extra instructions.
 * 
 * For example, assume we isolate a hot loop. If
 * x is live through the hot loop and has many uses,
 * it may be profitable to isolate the hot loop and assign
 * x to register when the reg pressure is low.
 * If x is live through the hot loop but
 * has few uses, it may be profitable to spill x during
 * the loop to relieve reg pressure.
 */

int calc_spill_weight(LiveRange *lr) {
    /* Proportional to the number of uses and defs. */
    int total = 0;
    /* TODO: loop through each interval when invoked? */
    for each interval {
        total += execution_weight * (uses + defs);
    }
    return total;
}

int calc_reg_pressure(LiveRange *lr) {
    int total = 0;
    for each interval {
        if (live vregs < threshold) continue;
        /* Estimate spill weight due to reg pressure
         * as the estimated average spill weight of
         * all live-in vregs. */
        total += execution_weight * (total uses + total defs across vregs) / live vregs;
    }
    return total;
}

/* Two strategies at play: spill the range
 * to relieve reg pressure, or add to register
 * to relieve spill cost. Take the best strategy. */
int calc_gross_profit(LiveRange *lr) {
    int profit = calc_spill_weight(lr) - calc_reg_pressure(lr); /* TODO - cache spill weight */
    return profit > 0 ? profit : -profit;
}

strategy try_hot_loop_splitting(LiveRange *lr) {

    /* TODO: find all loops complete covered by the range */

    strategy st;
    st.profit = 0;
    st.strategy = NO_SPLIT;

    for each loop { /* TODO - cache new instruction cost */
        int tmp_profit = calc_gross_profit(loop);
        for (p in loop_head->predecessors) {
            if (x in p->live_out) tmp_profit -= execution_weight of p;
        }
        for (e in loop->exits) {
            Block *s = e->bl0 out of loop ? e->bl0 : e->bl1;
            if (x in s->live_in) tmp_profit -= execution_wegith of s;
        }
        if (tmp_profit > st.profit) {
            st.loop = loop;
            st.profit = tmp_profit;
            st.strategy = HOT_LOOP;
        }
    }

    return profit, st; /* TODO - cache */
}

int try_inter_block_splitting(LiveRange *lr) {

    strategy st;
    st.profit = 0;
    st.strategy = NO_SPLIT;

    int total_uses = 0;
    int total_defs = 0;
    int total_blocks = 0;
    int new_spill_cost = 0;
    bool has_use = false;
    size_t last_block = SIZE_MAX;
    size_t curr_block = SIZE_MAX;

    for each interval {
        curr_block = interval.block;
        if (interval.block_ind != last_block) {
            new_spill_cost += (has_use ? 2 : 0) * last_block.execution_weight;
            total_blocks++;
            has_use = false;
        }
        if (interval.uses or interval.defs) {
            has_use = true;
            total_use += interval.uses;
            total_def += interval.defs;
        }
    }

    /* Eligibility: sparse uses or defs across blocks */
    int sparcity = (total uses + total defs) / total_blocks; /* TODO - cache */
    if (sparcity >= threshold) return st;

    /* TODO - cache spill cost */

    /* Assess profitability of splits */
    int profit;
    vector<Interval> intervals;
    vector<LiveRange> ranges;
    for each interval {
        if (prev interval is in different block) {
            intervals.clear();
        }

        create new interval from first and last use or def;
        push to intervals;

        if (next interval is in different block) {
            create live range; ranges.push();
            profit += calc_profit(new live range);
        }
    }
    if (!intervals.empty()) {
        create live range; ranges.push();
        profit += calc_profit(new live range);
    }

    if (profit > 0) {
        st.profit = profit;
        st.strategy = INTER_BLOCK;
        st.ranges = ranges;
    }

    return st;
}

int, Array<int> try_intra_block_splitting(LiveRange *lr) {
    /* Eligibility: contains at most one block,
     * and must have at least 3 uses. */
    if (not eligible) return 0;

    int profit = 0;
    int split_pt = 0;

    /* Try to split before each use or def and
     * assess profitability. */
    /* TODO: find interval, not just a single split point
     * find interval with high pressure but sparse use */
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
 * then for each split candidate block, create a new range. */
void split_around_block(LiveRange *lr, Strategy) {
    for each new range {
        create a new vreg;

        if (x live at new range) insert MOV vreg, x;
        if (x live after new range) insert MOV x, vreg;

        for each new interval {
            rewrite x to new vreg in instructions;
            update interval index based on MOV instructions;
            remove interval from original interval;
            update uses, defs, etc.;
        }
    }
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

/* Vreg rewrite + spill code gen.
 * TODO: need proof that, to ensure correctness,
 * it is sufficient to ensure that all vregs on live_out
 * are assigned to their allocated regs / spill
 * on block exit. That way, we can safely assume
 * that all vregs on live_in are assigned to their
 * allocated regs / spill on block entry.
 * 
 * E.g., in the following example, both block
 * ordering renders correct code because y, z
 * cannot share the same register:
 * 
 *  y = 1
 *  |    \
 * z = 2 |
 *  |    /
 *  x = y
 * 
 * y = 1
 * if cond goto L1
 * z = 2
 * L1: x = y
 * 
 * y = 1
 * if cond goto L2
 * L1: x = y
 * ...
 * L2: z = 2
 * goto L1
 * */
for each block {
    initialize assignment for each live_in reg (vreg -> register);

    for (each instruction) {

        if (MOV instruction) {
            if (RHS, LHS not spilled, and RHS == LHS) continue;
            if (RHS spilled, LHS not) {
                emit LD assignment[LHS], RHS.location;
                continue;
            }
            if (LHS spilled, RHS not) {
                emit ST LHS.location, assignment[RHS];
                continue;
            }
            else {
                change to LD;
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
            if there exists a free register, or if any RHS vreg is dead after
                set assignment[vreg] = register
            else if a RHS variable is spilled
                set assignment[vreg] = assignment[rhs vreg]
            else
                find any register, evict the variable held
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

    /* By design, all live_outs are in their proper place already. */
}
