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
    slot 1 - inst A
    slot 2 - inst B
    slot 3 - inst C
    slot 4

    slot 1 - inst A
    slot 2 - inst B
    slot 3 - MOV x1, x
    slot n - inst C
    slot 4

    x's live range is reset to [..., 3].
    x1's live range is set to [4, ...].
    For other variables, live range does not change.
    Slot n will be reindexed to 4.
    Slot 5 will be reindexed to 5.
 * 
 * Decision: interval by block boundaries instead of
 * across blocks. Control flow is easier to reason about
 * and MOV insertion accompanies by block insertion
 * is easier to implement. This is different from LLVM.
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

#include "arena.h"
#include "array.h"
#include "pqueue.h"
#include "inst.h"
#include "block.h"

#define MIN_SPLIT_INTERVAL_SIZE 3
#define MIN_SPLIT_INTER_BLOCK_SPARCITY_THRES 2

typedef struct {
    Arena var_info;
    Array var_to_info;  // var ind to VarInfo *

    /* Mapping from inst->ind to Slot object. */
    Arena slots;
    Array inst_to_slot;  // fixed size INST_POOL_SIZE
    Array block_to_slots;  // fixed size blocks->size; block->ind to DLL; TODO - in block info?

    Arena loop_info;
    Array loop_to_info;

    Arena block_info;
    Array block_to_info;
} RegAllocContext;

typedef struct {
    DLLElem *prev;
    DLLElem *next;
    size_t ind;  // Represents local order in a block.
    Inst *inst;  // The instruction right after the slot.
} Slot;

// TODO: sorted array of intervals, name it TBD

typedef struct {
    Slot *start;
    Slot *end;
    Block *block;
} LiveInterval;

typedef struct {
    enum { NO_SPLIT, HOT_LOOP, INTER_BLOCK, INTRA_BLOCK } type;
    int profit;
    Array interval_sets;  // Array of TBD *
} SplitStrategy;

// Reg -> interval assignment mapping
TBD *reg_intervals[NUM_REG]; // TODO - cranelift uses a btree

typedef struct {
    Var *var;
    TBD intervals;  // Live intervals for a var
    Reg assigned_reg;  // 0 = spill
    int spill_weight;  // SIZE_MAX = unset
} VarInfo;

typedef struct {
    DLL slots;
    Loop *loop;
} BlockInfo;

typedef struct {
    Loop *loop;
    TBD intervals;
    int execution_weight;
} LoopInfo;

void insert_live_interval_set(TBD *array, LiveInterval *interval) {

}

void remove_live_interval_set(TBD *array, LiveInterval *interval) {

}

/* Returns true if l1 comprises l2. */
bool comprise_live_interval_set(TBD *l1, TBD *l2) {

}

/* Returns true if two live intervals interfere. */
bool has_interference_ii(LiveInterval *i1, LiveInterval *i2) {
    if (!i1 || !i2) return false;
    if (i1->block != i2->block) return false;
    return i1->end->ind >= i1->start->ind && i1->start->ind <= i2->end->ind;
}

/* Returns true if two live interval sets interfere. */
bool has_interference_ss(TBD *r1, TBD *r2) {
    // TODO
}

/* Returns true if a live interval set interferes with a live interval. */
bool has_interference_si(TBD *lr, LiveInterval *interval) {

}

/* Given a VarInfo and an interval set, find the set of
 * VarInfo's that interfere with the interval. */
Array *query_interference_vs(VarInfo *info, TBD *l) {
    // TODO
}

/* Given two live interval sets l1 and l2, find all interfering
 * live intervals. */
TBD *query_interference_ss(TBD *l1, TBD *l2) {
    Set<LiveRange *>ranges;

    for each interval in lr {
        /* TODO - quickly find location in reg range that
         * matches with interval.start, then
         * continuing to walk until an interval intersects or
         * reg_interval.start >= interval.end */
        Array<Interval> intervals = query_interference(occupied[r], interval);
        for (interval in intervals) {
            ranges.add(interval var range);
        }
    }

    return ranges;
}

TBD *query_interference_si(TBD *lr, LiveInterval *interval) {

}

/* Split v2 from v1 and save the result to v3. */
// TODO: tag new use or def
void diff_var_info(VarInfo *v1, VarInfo *v2, VarInfo *v3) {

}

int priority_func(VarInfo *info) {
    /* TODO: Take into account spill cost, loop
     * depth, number of uses / defs, range size, etc. */
}

/* Given live intervals, find a free reg that can fit
 * the live intervals. */
// TODO - consider better search method than linear?
Reg find_free_reg(TBD *lr, Array *reg_intervals) {
    for (uint8_t r = 0; r < NUM_REG; r++) {
        if (has_interference_ss(lr, reg_intervals[r]))
            continue;
        return (Reg) r;
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

/* Given a program point and a vreg, return true if vreg
 * is live at the program point. */
bool is_live(Var *var, Slot *slot) {
    // TODO
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

void allocation(Array *blocks, LivenessDFAObject *objs) {
    RegAllocContext *c = init_reg_alloc_context();

    /* Create live intervals and slots. */
    init_alloc(blocks, objs, c);

    // TODO
    precolor registers;

    /* Coalescing pre-pass. */
    coalescing(blocks, c);

    /* Push each var_info to priority queue. */
    PQueue worklist;
    for (var_info in var_to_info) {
        if (var_info is not null) insert_pq(&var_info, &worklist);
    }

    while (worklist.size) {
        VarInfo *info = (VarInfo *) top_pq(&worklist);

        /* Try assigning intervals to a free register. */
        Reg reg = find_free_reg(info->intervals, reg_intervals);
        if (reg != SPILLED) {
            info->assigned_reg = reg;
            continue;
        }

        /* Find the first register where the eviction
         * cost of assigned intervals that conflict
         * with the incoming intervals is lower than the
         * spill weight of the incoming intervals, evict the
         * conflicting ranges, and assign the incoming
         * range. */
        else {
            bool assigned = false;
            for (int r = 0; r < NUM_REG; r++) {
                Array *var_info_arr = find_interfering_var_info(info, reg_intervals[r]);
                int eviction_cost = 0;
                for (distinct var_info in var_info_arr) {
                    eviction_cost += calc_spill_weight(var_info);
                }

                if (eviction_cost < calc_spill_weight(info)) {
                    for (distinct var_info in var_info_arr) {
                        var_info->assigned_reg = REG_NA;
                        push_pq(&var_info, &worklist);
                    }
                    info->assigned_reg = (Reg) r;
                    assigned = true;
                    break;
                }
            }
            if (assigned) continue;
        }

        /* Try splitting the incoming intervals
         * for better assignment opportunities, and
         * add the new intervals to queue. Note that
         * this cannot result in infinite recursion
         * since split intervals can only be smaller,
         * and at some point, intervals cannot be split
         * further. */
        SplitStrategy p1, p2, p3;
        SplitStrategy winner;

        p1 = try_hot_loop_splitting(info);
        p2 = try_intra_block_splitting(info);
        p3 = try_long_range_splitting(info);
        winner = p1.profit > p2.profit ? p1 : p2;
        winner = winner.profit > p3.profit ? winner : p3;

        if (winner.profit) {
            switch (winner.type) {
                case HOT_LOOP:
                    ranges = split_around_loop(info, winner);
                    push ranges to queue;
                    break;
                case INTER_BLOCK:
                    ranges = split_around_block(info, winner);
                    spill background range;
                    push foreground ranges to queue;
                    break;
                case INTRA_BLOCK:
                    ranges = split_within_block(info, winner);
                    spill foreground ranges;
                    push background range to queue;
                    break;
                default:
            }
            continue;
        }
    
        /* Spill the live range if no other allocation
        * methods apply. */
        info->assigned_reg = SPILLED;
    }

cleanup:
    free_reg_alloc_context(c);
}

RegAllocContext *init_reg_alloc_context(void) {
    RegAllocContext *c = malloc(sizeof(RegAllocContext));
    // TODO
    return c;
}

/* Create live intervals for each vreg variable. Create slots. */
void init_alloc(Array *blocks, LivenessDFAObject *objs, RegAllocContext *c) {
    /* Build live intervals from liveness state. */
    Array *var_to_info;  // VAR_POOL_SIZE

    for each block {
        initialize slot based on live out;

        for each live out var {
            check if var_info already exists, if not, create new;
            if (vreg live after) {
                create new interval with start = slot, end = slot;
                vreg curr_interval = interval;
            }
        }

        for each instruction {
            upate liveness state;
            create new slot;
            for each live var at new slot {
                check if var_info already exists, if not, create new;
                if (vreg live after and live before) extend start in curr_interval to slot;
                else if (vreg dead after and live before) {
                    create new interval for var; add to var intervals;
                    start slot = slot before; end slot = slot before;
                    vreg curr_interval = interval;
                }
            }
        }

        for each var in dict {
            add corresponding var_info to pq;
        }
    }
}

/* Scan all copy instructions and merge non-overlapping
 * intervals. */
void coalescing(Array *blocks, RegAllocContext *c) {
    for (block in blocks) {
        for each inst {
            if (inst is a copy inst, aka MOV r1, r2) {
                Var *lhs = inst->lhs.var;
                Var *rhs = inst->rhs.var;
                VarInfo *l_info = c->var_to_info[lhs->ind];
                VarInfo *r_info = c->var_to_info[rhs->ind];

                if (
                    overlap_live_interval_set(l_info->intervals, r_info->intervals)
                ) continue;

                /* Merge the rhs variable into the lhs variable. */
                merge_live_intervals(l_info->intervals, r_info->intervals);
                // TODO: need to query uses and defs in var info to get all occurances?
                for (all occurrences of rhs vreg) {
                    rename to lhs var;
                }
                c->var_to_info[rhs->ind] = NULL;
            }
        }
    }
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

int calc_spill_weight(VarInfo *info) {
    // TODO: If cached, just return cache
    int total = 0;
    for each use {
        total += execution_weight of use blocks * 2;
    }
    for each def {
        total += execution_weight of def blocks;
    }
    return total;
}

/* Estimate reg pressure of a VarInfo by
 * taking the avg cost of evicting conflicting intervals
 * from an existing register. */
int calc_reg_pressure(VarInfo *info) {
    int total_eviction_cost = 0;
    int total_regs;

    for each reg {
        TBD *intervals = query_interference_vs(info->intervals, reg_intervals);
        if (intervals not empty) total_regs++;
        total_eviction_cost += calc_spill_weight(info);
    }

    if (total_regs < NUM_REG) return 0;

    return total_eviction_cost / total_regs;
}

/* Estimate profit of splitting interval sets from
 * orig->intervals, without considering the cost
 * of MOV insertion, with two strategies at play:
 * estimate the profit of spilling split intervals
 * to relieve reg pressure, or the profit of
 * assigning split intervals to register.
 * Take the greater value of the two. */
// TODO: update logic for interval sets
int calc_profit_without_mov_insertion(VarInfo *orig, Array *interval_sets) {
    int baseline = calc_spill_weight(orig);

    VarInfo split_info = {orig->var, split_intervals, NO_REG, ...};
    VarInfo new_info;
    diff_var_info(orig, &split_info, &new_info);

    int split_spill_weight = calc_spill_weight(&split_info);
    int split_reg_pressure = calc_reg_pressure(&split_info);
    int new_spill_weight = calc_spill_weight(&new_info);
    int new_reg_pressure = calc_reg_pressure(&new_info);

    int s1 = baseline - (split_spill_weight + new_reg_pressure);
    int s2 = baseline - (new_spill_weight + split_reg_pressure);
    int profit = s1 > s2 ? s1 : s2;

    return profit > 0 ? profit : 0;
}

/* Strategy for hot loop splitting:
 * If a var interval set covers a hot loop completely,
 * try splitting out the hot loop region. If the
 * interval set covers multiple loops, select the loop
 * that yields the greatest profit from splitting. */
// TODO - tag use and def
SplitStrategy try_hot_loop_splitting(VarInfo *info) {
    SplitStrategy res;
    res.profit = 0;
    res.type = NO_SPLIT;

    Array interval_sets;  // fixed size of 1 interval_set

    for (each interval in info->intervals) {
        get loop;
        if (loop is already assessed) continue;
        if (
            !comprise_live_interval_set(
                info->intervals,
                loop_info->intervals
            )
        ) {
            add loop to assessed;
            continue;
        }

        ((TBD *) interval_sets.arr)[0] = (TBD *) loop->intervals;
        int tmp_profit = calc_profit_without_mov_insertion(info, interval_sets);
        if (!tmp_profit) continue;

        /* Adjust est. profit by subtracting the execution weights
         * of predecessors where the variable is live out, and
         * successors where the variable is live in. This is because
         * MOV instructions would be inserted after these predecessors
         * and before these successors, costing 1 cycle for each iteration. */
        for (p in loop->head->predecessors) {
            if (p in loop) continue;

            get slot at live out of p;
            if (is_live(info->var, slot))
                tmp_profit -= execution weight of p;
        }

        for (e in loop->exits) {
            Block *s = e->bl0 out of loop ? e->bl0 : e->bl1;
            get slot at live in of s;
            if (is_live(inf->var, slot))
                tmp_profit -= execution_weight of s;
        }
        if (tmp_profit > res.profit) {
            ((TBD *) res.interval_sets.arr)[0] = (TBD *) loop->intervals;
            res.profit = tmp_profit;
            res.type = HOT_LOOP;
        }
    }

    free(interval_sets.arr);
    return res;
}

/* Strategy for inter block splitting:
 * For all intervals that cover entire blocks,
 * if the block has uses or defs, carve out a
 * new interval starting from the first use / def and end
 * with the last use / def, then spill the background
 * interval. Note that every split interval can be
 * assigned a new variable. */
SplitStrategy try_inter_block_splitting(VarInfo *info) {
    SplitStrategy res;
    res.profit = 0;
    res.type = NO_SPLIT;

    int total_uses = 0;
    int total_defs = 0;
    int total_blocks = 0;
    bool has_use = false;
    Block *last_block = NULL;

    for each interval {
        if (interval->block != last_block) {
            total_blocks++;
            last_block = interval->block;
        }
        if (interval->uses.size or interval->defs.size) {
            total_uses += interval->uses.size;
            total_defs += interval->defs.size;
        }
    }

    /* Only apply the splitting if the uses and defs
     * across blocks are sparse. */
    int sparcity = total_blocks / (total_uses + total_defs);
    if (sparcity < MIN_SPLIT_INTER_BLOCK_SPARCITY_THRES) return st;

    /* Assess profitability of splits. */
    int cost = 0;
    Array interval_sets;

    for each interval {
        if (interval size < MIN_SPLIT_INTERVAL_SIZE) continue;
        if (interval does not cover entire block and have use or def) {
            continue;
        }

        LiveInterval *interval = create new interval from first and last use or def;
        cost += (1 + is_live(info->var, slot after last use or def) ? 1 : 0) * execution_weight;

        TBD *intervals = push interval to set;
        tag uses and defs in intervals;
        push intervals to interval_sets;
    }

    if (!interval_sets.size) return res;

    int profit = calc_profit_without_mov_insertion(info, intervals) - cost;

    if (profit > 0) {
        res.profit = profit;
        res.interval_sets = interval_sets;
        res.type = INTER_BLOCK;
    }

    return res;
}

/* Strategy for intra block splitting:
 * If local reg pressure is high within a block,
 * carve out an interval for the high pressure
 * region. Note that every split interval can
 * be assigned a new variable. */
SplitStrategy try_intra_block_splitting(VarInfo *info) {
    SplitStrategy res;
    res.profit = 0;
    res.type = NO_SPLIT;

    Array interval_sets;
    int cost = 0;

    for (each interval) {
        if (interval size < MIN_SPLIT_INTERVAL_SIZE) continue;

        Slot *start = NULL;
        Slot *end = NULL;

        while having next use or def {
            if (is a use) num_uses++;
            slot = slot after def or before use;
            if (start == SIZE_MAX && slot reg pressure is high) {
                start = slot;
            }
            else if (start != SIZE_MAX && slot reg pressure is low) {
                end = slot;

                LiveInterval new_interval = {start, end, interval->block};
                TBD *intervals = push new_interval to set;
                add uses and defs;
                add intervals to interval_sets;

                cost += (
                    (uses or defs ? 1 : 0) + (is_live(lr_vreg, end) ? 1 : 0)
                ) * execution_weight;

                start = NULL;
            }
        }

        if (start) {
            end = interval end;

            LiveInterval new_interval = {start, end, interval->block};
            TBD *intervals = push new_interval to set;
            add uses and defs;
            add intervals to interval_sets;

            cost += (
                (uses or defs ? 1 : 0) + (is_live(lr->vreg, end) ? 1 : 0)
            ) * execution_weight;
        }
    }

    if (!interval_sets.size) return res;

    int profit = calc_profit_without_mov_insertion(info, interval_sets) - cost;

    if (profit > 0) {
        res.profit = profit;
        res.interval_sets = interval_sets;
        res.type = INTRA_BLOCK;
    }

    return res;
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
                    // TODO: handle RHS in conditional branch inst, cannot evict and reload
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
