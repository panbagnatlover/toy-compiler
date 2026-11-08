/*
 * Decision: do coalescing as a greedy pre-pass.
 * Based on the paper, greedy heuristics in a separate
 * pass does not generate materially different
 * results from the optimal integrated solution.
 * Need to implement splitting to account for the
 * drawback of register pressure as a result of coalescing.
 * 
 * Decision: split and insert MOV inst on the fly.
 * vreg -> rewrite and spill code generation in a post-pass.
 * See comment at post-pass for why it correctly generates
 * code.
 * 
 * Decision: no need to maintain reg pressure at every slot.
 * Reg pressure estimated by eviction cost of interfering
 * ranges.
 * 
 * Decision: Slot object to specify slot locations.
 * Note the following example. If using slot object,
 * in case of MOV insertion, can "renumber" a slot
 * in a block without needing to update each interval
 * object.
    slot 1
    inst A
    slot 2
    inst B
    slot 3
    inst C
    slot 4

    slot 1
    inst A
    slot 2
    inst B
    slot 3
    MOV x1, x
    slot 4 - x dead, x1 live
    inst C
    slot 4 - renamed to 5

    Need to split x interval at slot before MOV (i.e.,
    insertion point), and have x1 interval start at
    slot after MOV.
 * 
 * TODO: b tree?
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

/* Given a live range, find a free register that
 * can fit the live range. */
Reg find_free_reg(LiveRange *lr) {
    for each register {
        bool assigned = true;
        for each interval in lr->intervals {
            if (occupied[register].has_interference(interval)) {
                assigned = false;
                break;
            }
            if (!assigned) break;
        }
        if (assigned) return register;
    }
    return SPILLED;
}

/* Given a program point, find all free regs. */
Array<Reg> find_free_regs_at_point(size_t block_ind, size_t slot_ind) {
    Array<Reg> regs;
    for each register {
        if (!occupied[register].has_interference(program point)) {
            regs.add(register);
        }
    }
    return regs;
}

/* Given a live range and a register, find all interfering
 * live ranges. */
Set<LiveRange *> find_interfering_ranges(LiveRange *lr, Reg r) {
    Set<LiveRange *>ranges;

    for each interval in lr {
        /* TODO - quickly find location in reg range that
         * matches with interval.start, then
         * continuing to walk until an interval intersects or
         * reg_interval.start >= interval.end */
        Array<Interval> intervals = occupied[r].query_interference(interval);
        for (interval in intervals) {
            ranges.add(interval var range);
        }
    }

    return ranges;
}

/* Given a program point, find all intersecting intervals. */
Set<Interval *> find_intersecting_intervals(size_t block_ind, size_t slot_ind) {
    /* TODO */
}

/* Given a program point and a vreg, return true if vreg
 * is live at the program point. */
bool is_live(Slot *slot, VReg *vreg) {
    /* TODO - use .find() to get the range point,
     * return true if intersects an interval. */
    return vreg range.has_interference([slot, slot]);
}

void insert_mov_instruction(Slot *slot, Vreg *old_vreg, Vreg *new_vreg) {
    Inst *mov = MOV new_vreg, old_vreg;
    Inst *after_mov = slot->inst;
    Inst *before_mov = after_mov->prev;

    /* Set up linkage with existing instructions. */
    mov->next = after_mov;
    mov->prev = before_mov;
    after_mov->prev = mov;
    if (before_mov) before_mov->next = mov;

    /* Create a new slot and set up linkage. */
    Slot *after_slot = slot->next;
    Slot *before_slot = slot;
    slot = new Slot(after_slot->ind);

    slot->next = after_slot;
    slot->prev = before_slot;
    if (after_slot) after_slot->prev = slot;
    before_slot->next = slot;

    /* Set up linkage between slot and instruction. */
    before_slot->inst = mov;
    slot->inst = after_mov;

    /* TODO: do the same in reverse for inst_info. */
    
    /* Renumber subsequent slots after insertion point. */
    Slot *curr = after_slot;
    while (curr) {
        curr->ind++;
        curr = curr->next;
    }
}

/* TODO - need to support fast eviction, fast insertion,
 * .find() query */
/* register -> interval assignment mapping. */
Array<int, SortedIntervalObject>;  // TODO - cranelift uses a btree
precolor registers;
merge vregs that can be colaesced with no interference,
update live ranges;

push live ranges onto priority queue;

while (queue not empty) {
    get front;

    /* Try assigning incoming range to a free register. */
    Reg reg = find_free_reg(front);
    if (reg) {
        front->register = reg;
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
            Set<LiveRange *> ranges = find_interfering_ranges(front, register);
            int eviction_cost = 0;
            for (range in ranges) eviction_cost += range->spill_weight;

            if (eviction_cost < front->spill_weight) {
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
                spill foreground ranges;
                push background range to queue;
                break;
            default:
        }
        continue;
    }
 
    /* Spill the live range if no other allocation
     * methods apply. */
    front->assigned_reg = SPILLED;
}

/* Estimate split profitability as 
 * abs(spill weight of the split region, minus
 * estimated eviction cost due to reg pressure), minus
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
    int total = 0;
    for each interval {
        total += execution_weight * (uses + defs);
    }
    return total;
}

/* If free register, min_eviction_cost = 0;
 * otherwise, avg cost of evicting the conflicting
 * range from an existing register. */
int calc_reg_pressure(LiveRange *lr) {
    int total_eviction_cost = 0;
    int total_regs;

    for each reg {
        Set<LiveRange *> ranges = find_interfering_ranges(lr, register);
        if (ranges not empty) total_regs++;
        for (range in ranges) total_eviction_cost += range->spill_weight;
    }

    if (total_regs < threshold) return 0;

    return total_eviction_cost / total_regs;
}

/* Two strategies at play: spill the range
 * to relieve reg pressure, or add to register
 * to relieve spill cost. Take the best strategy. */
int calc_profit_without_mov_insertion(LiveRange *orig_lr, LiveRange *split_lr) {
    int baseline = orig_lr->spill_weight;

    LiveRange *new_lr; /* Cut split_lr from orig_lr */
    int split_spill_weight = calc_spill_weight(split_lr);
    int split_reg_pressure = calc_reg_pressure(split_lr);
    int new_spill_weight = calc_spill_weight(new_lr);
    int new_reg_pressure = calc_reg_pressure(new_lr);

    int s1 = baseline - (split_spill_weight + new_reg_pressure);
    int s2 = baseline - (new_spill_weight + split_reg_pressure);
    int profit = s1 > s2 ? s1 : s2;

    return profit > 0 ? profit : 0;
}

/* TODO - cache spill weight on range creation each time */

strategy try_hot_loop_splitting(LiveRange *lr) {

    /* TODO: find all loops completely covered by the range */

    strategy st;
    st.profit = 0;
    st.strategy = NO_SPLIT;

    for each loop {
        int tmp_profit = calc_profit_without_mov_insertion(lr, loop);
        if (!tmp_profit) continue;

        for (p in loop_head->predecessors) {
            if (is_live(x, p_info.end)) tmp_profit -= execution_weight of p;
        }
        for (e in loop->exits) {
            Block *s = e->bl0 out of loop ? e->bl0 : e->bl1;
            if (is_live(x, s_info.start)) tmp_profit -= execution_weight of s;
        }
        if (tmp_profit > st.profit) {
            st.loop = loop;
            st.profit = tmp_profit;
            st.strategy = HOT_LOOP;
        }
    }

    return profit, st;
}

strategy try_inter_block_splitting(LiveRange *lr) {

    strategy st;
    st.profit = 0;
    st.strategy = NO_SPLIT;

    int total_uses = 0;
    int total_defs = 0;
    int total_blocks = 0;
    bool has_use = false;
    size_t last_block = SIZE_MAX;
    size_t curr_block = SIZE_MAX;

    for each interval {
        curr_block = interval.block;
        if (interval.block_ind != last_block) {
            total_blocks++;
            has_use = false;
        }
        if (interval.uses or interval.defs) {
            has_use = true;
            total_uses += interval.uses;
            total_defs += interval.defs;
        }
    }

    /* Eligibility: sparse uses or defs across blocks */
    int sparcity = (total_uses + total_defs) / total_blocks;
    if (sparcity >= threshold) return st;

    /* Assess profitability of splits */
    int cost = 0;
    vector<Interval> intervals;
    vector<Range> ranges;
    for each interval {
        if (interval size < threshold) continue;
        if (interval does not cover entire block and have use or def) intervals.add(interval);

        create new interval from first and last use or def;
        cost += (1 + is_live(vreg, slot after last use or def) ? 1 : 0) * execution_weight;
        push to intervals;

        create new range from interval;
        push to ranges;
    }

    if (intervals.empty()) return st;

    profit = calc_profit_without_mov_insertion(lr, intervals) - cost;

    if (profit > 0) {
        st.profit = profit;
        st.strategy = INTER_BLOCK;
        st.ranges = ranges;
    }

    return st;
}

strategy try_intra_block_splitting(LiveRange *lr) {

    strategy st;
    st.profit = 0;
    st.strategy = NO_SPLIT;

    Array<Interval> intervals;
    Array<Range> ranges;

    int cost;

    for each interval {
        if (uses or size < threshold) continue;

        size_t start = SIZE_MAX;
        size_t end;

        while having next use or def {
            slot = slot after def or before use;
            if (start == SIZE_MAX && slot reg pressure is high) {
                start = slot;
            }
            else if (start != SIZE_MAX && slot reg pressure is low) {
                end = slot;

                create interval from [start, end);
                intervals.add(interval);

                create range from interval;
                ranges.add(range);

                cost += (
                    (uses or defs ? 1 : 0) + (is_live(lr_vreg, end) ? 1 : 0)
                ) * execution_weight;

                start = SIZE_MAX;
            }
        }

        if (start != SIZE_MAX) {
            end = interval end;

            create interval from [start, end);
            intervals.add(interval);

            create range from interval;
            ranges.add(range);

            cost += (
                (uses or defs ? 1 : 0) + (is_live(lr->vreg, end) ? 1 : 0)
            ) * execution_weight;
        }
    }

    int profit = calc_profit_without_mov_insertion(lr, intervals) - cost;

    if (profit > 0) {
        st.profit = profit;
        st.ranges = ranges;
        st.strategy = INTRA_BLOCK;
    }

    return st;
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

    initiate new live range for x1;

    for each loop block {
        rewrite every x in loop to x1;
        remove interval from live range of x;
        add interval to live range for x1;
    }

    for p in loop_head->predecessors {
        if (p->innermost_loop == loop) continue;
        if (x not live_out in p) continue;

        if (p->bl1) {
            Block *b = new Block();

            if (p->bl1 == loop_head) p->bl1 = b;
            else p->bl0 = b;

            b->bl0 = loop_head;
            b->start = MOV x1, x;
            b->end = b->start;

            /* TODO - Update block_info. */
            create block info for b;

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

            /* TODO: update block_info. */
            create block info for b;

            /* Update live range. */
            add the block as end to live range of x1.

            for all vregs in l.live_out intersect s.live_in {
                add the block to live range of vreg;
            }

            /* TODO - update use, def */
        }
        else {
            insert MOV x, x1 at the beginning of s;

            /* Update live range. */
            for all vregs in s.live_in {
                extend interval by 1;
            }

            /* TODO - update use, def */
        }
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

        /* Note that each new range consists of one interval
         * by design. */

        if (x live at new interval) insert MOV vreg, x;
        if (x live after new interval) insert MOV x, vreg;

        rewrite x to new vreg in instructions;
        update uses, defs, etc.;
    }
}

/* Intra block splitting.
 * Insert MOV at the first interval boundary,
 * then, if original vreg live after the last boundary,
 * insert MOV at the last boundary.
 * No change to live in, live out sets at the block level. */
void split_within_block(LiveRange *lr, strategy) {
    for each new range in strategy {
        for each interval {
            insert MOV x1, x at first splitting point;
            if x live at second splitting point, insert MOV x, x1
            at second splitting point;

            for each instruction in between:
                allocate to new index;
                replace x with x1;
            
            split live range of x for that block:
                end at first splitting point;
                if second splitting point, start after the second MOV;

            /* TODO: update use defs */
        }
    }
}

/* TODO: evicted vregs go into its own stack loc, or a
 * dedicated memory loc set aside? */
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

    size_t slot = 0;

    for (each instruction) {

        if (LHS is dead in slot after) remove instruction, continue;

        if (MOV instruction) {
            if (RHS == LHS) remove instruction, continue;
            if (RHS not spilled, LHS not spilled) {
                rewrite MOV assignment[LHS], assignment[RHS];
                continue;
            }
            if (RHS spilled, LHS not spilled) {
                rewrite LD assignment[LHS], RHS.location;
                continue;
            }
            if (LHS spilled, RHS not spilled) {
                rewrite ST LHS.location, assignment[RHS];
                continue;
            }
            else {
                if free register {
                    rewrite to LD register, RHS.location;
                    insert ST register, LHS.location;
                }
                else {
                    take any register, get vreg in slot before;
                    insert ST register, vreg.location;
                    rewrite to LD register, RHS.location;
                    insert ST register, LHS.location;
                    insert LD register, vreg.location;
                }
            }

            continue;
        }

        tmp = Map();
        evicted = Set();

        free_regs = get free registers at slot before;

        for each RHS vreg

            Reg register = assignment[RHS];

            if (register) rewrite vreg to register in instruction;

            else
                if (RHS == tmp[vreg] for another assigned vreg RHS)
                    register = tmp[vreg];
                    rewrite vreg to register;
                else if (free_reg not empty)
                    register = first free reg;
                    insert LD register, vreg.location;
                    rewrite vreg to register in instruction;
                    tmp[vreg] = register;
                    free_reg.remove(register);
                else
                    /* Note that if a vreg is live in slot before
                     * but dead after, it must be used in the current
                     * instruction. So, when evicting vregs, we don't
                     * need to consider the case where an evicted vreg
                     * can be dead after. */
                    find a register where assignment[RHS] != register and tmp[RHS] != register for any RHS, and evict the current held vreg;
                    insert ST register, evicted vreg location;
                    evicted.add(evicted reg);
                    insert LD register, vreg;
                    rewrite vreg to register in instruction;
                    tmp[vreg] = register;

        Reg register = assignment(LHS);

        if (register) {
            rewrite vreg to register in instruction;
        }
        else 
            if free_reg not empty
                register = first free reg;
                free_reg.remove(register);
            else if any unspilled vreg is live in slot before and dead in slot after
                register = assignment[vreg];
            else if a RHS variable is spilled /* can be itself */
                register = tmp[RHS];
            else
                find any register where assignment[RHS] != register and tmp[RHS] != register, evict the variable held
                insert ST register, evicted vreg;
                evicted.add(evicted reg);

            rewrite LHS to register;
            insert ST register, vreg.location;

        for each evicted vreg
            insert LD assignment[vreg], vreg.location;

        slot++;
    }

    /* By design, all live_outs are in their proper place already. */
}
