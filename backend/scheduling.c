/* TODO: 2nd round scheduling does not require
 * updating live intervals or being register aware.

 * Do not rely on use def chain since it won't be preserved
 * after reg alloc.

 * Need to run live variable analysis before 1st round
 * scheduling. Build intervals after 1st round scheduling.
 * 
 * Set up slots before scheduling. Each instruction is tied
 * to a slot, and each slot ties back to an instruction.
 * 
 * Approach:
 * Each instruction type has a static execution profile
 * with the following params: cycle, resource type, # units.
 * In addition, each vreg / vmem has a latency with respect to
 * its instruction.
 * 
 * These params and metrics are merely abstractions and do
 * not represent how hardware actually executes an instruction.
 * However, this representation is helpful for software instruciton
 * scheduling. Metrics can be inferred from observed hardware
 * behavior.
 */
#include "dict.h"
#include "dll.h"
#include "vreg.h"
#include "vmem.h"
#include "block.h"
#include "instprofile.h"
#include "liveness.h"
#include "scheduling.h"

/* Lookback window for list scheduling
 * in a circular buffer. */
#define RESOURCE_LOOKBACK 10
#define BUFFER_SIZE MAX_CYCLES + RESOURCE_LOOKBACK
#define REG_THRESHOLD 30  // TODO: this is bs value rn

typedef struct {
    int depth;  // Longest latency path from entry to curr instruction
    int rp_diff;  // Est. incremental change to reg pressure
    int outdegree;  // Current number of unscheduled successors
    Dict pred_latencies;  // Predecessor latencies
    Dict succ_latencies;  // Successor latencies
    uint8_t res_demand[NUM_RESOURCE_TYPE];  // Total units demanded by resource
    size_t schedule;  // Relative time cycle of the scheduled instruction in a block
} InstInfo;

// TODO: map from inst->ind to inst placement order
// in block, then inst_info_arr follows the inst placement
// order
typedef struct {
    Array inst_info_arr;  // fixed size block->insts.size

    /* Remaining resource demand. Sum of all units needed
    * for a resource from all unscheduled instructions. */
    int rrd[NUM_RESOURCE_TYPE];

    /* Resource table. Implemented as a circular buffer. */
    int rt[BUFFER_SIZE * NUM_RESOURCE_TYPE];

    /* Mapping between instruction->ind to local
     * instruction order. This array is never reset
     * across blocks. */
    Array inst_order;
} BlockInfo;

typedef struct {
    /* Mapping from inst->ind to Slot object. */
    Array inst_to_slot;  // fixed size INST_POOL_SIZE
    Arena slots;
    Array block_to_slots;  // fixed size blocks->size; block->ind to DLL
} RegAllocContext;

BlockInfo *init_block_info(bool pre_alloc) {
    BlockInfo *block_info = malloc(sizeof(BlockInfo));

    block_info->inst_info_arr.arr = blabla;

    // TODO: malloc rrd, etc.

    if (!pre_alloc) {
        // TODO: malloc inst_info_map
    }

    return block_info;
}

void reset_block_info(Block *block, BlockInfo *block_info) {
    // TODO: resize block_info->inst_info_arr, reset dict

    for (int i = 0; i < NUM_RESOURCE_TYPE; i++) rrd[i] = 0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        for (int j = 0; j < NUM_RESOURCE_TYPE; j++) rt[i][j] = 0;
    }
}

void free_block_info(BlockInfo *block_info) {
    // free arrays

    free(block_info);
}

// Slot needs to have ind, inst *, live * bitarray.

void scheduling(Array *blocks, bool pre_alloc, RegAllocContext *c) {
    /* Run live variable analysis on vregs if scheduling
     * is executed before reg allocation. */
    if (pre_alloc) {
        LivenessContext *lc = liveness_vreg(blocks);
        LivenessDFAObject *objs = lc->objs->arr;

        /* Store liveness info in Slot objects. */
        for (int i = 0; i < blocks->size; i++) {
            Block *block = blocks[i];
            LivenessDFAObject *obj = &objs[i];
            BitArray *live = obj->out;

            /* Create a new slot object from the live out set. */
            Slot *slot = create_slot(c->slots);
            slot->live_set = live;
            slot->inst = NULL;
            push_dll_elem(slot, &(c->block_to_slots[block->ind]));

            BitArray *tmp_def = bitarray();
            BitArray *tmp_use = bitarray();

            /* For each instruction, create a corresponding
             * slot object from liveness information and
             * set up linkage. The index to each slot object
             * represents the order of the instruction in a
             * block. */
            for (each inst backwards) {
                set_def_vreg(inst, tmp_def);
                set_use_vreg(inst, tmp_use);
                live = tmp_use U (live - tmp_def);

                reset_bitarray(tmp_def);
                reset_bitarray(tmp_use);

                Slot *slot = create_slot(c->slots);
                slot->live_set = live;
                slot->inst = inst;
                c->inst_to_slot[inst->ind] = slot;
                insert_dll_elem(slot, NULL, &(c->block_to_slots[block->ind]);
            }
        }
    }

    BlockInfo *block_info = init_block_info(pre_alloc);

    for (int i = 0; i < blocks->size; i++) {
        Block *block = blocks[i];
        reset_block_info(block, block_info);

        /* Map local inst order within each block. */
        size_t counter = 0;
        for (each inst) {
            block_info->inst_order[inst->ind] = counter++;
        }

        /* Build DAG and record metadata. */
        buildDAG(block, block_info);

        /* Schedule instructions. */
        listScheduling(block, pre_alloc, block_info, c);
    }

    free_block_info(block_info);
}

/* Update latency info between prev and curr instructions. */
static inline void _update_latency_dict(int8_t *l, Inst *prev, Inst *curr, InstInfo *arr) {
    int8_t ol;
    size_t prev_ind = prev->ind;
    size_t curr_ind = curr->ind;

    Dict *dict = &arr[prev_ind].succ_latencies;
    get_dict(&curr_ind, &ol, dict);
    if (l > ol) {
        put_dict(&curr_ind, l, dict);
        dict = &arr[curr_ind].pred_latencies;
        put_dict(&prev_ind, l, dict);
    }
}

/* Calculate the number of cycles needed between
 * two instructions, based on when a vreg from
 * the prior instruction is ready for consumption
 * by a vreg from the current instruction. */
void build_latency_edge_vreg(VReg *prev, VReg *curr, InstInfo *arr) {
    InstExecutionProfile *prev_profile = get_inst_profile(prev->inst);
    InstExecutionProfile *curr_profile = get_inst_profile(curr->inst);

    int8_t l1 = prev_profile->mem_latencies[prev->ind];
    int8_t l2 = curr_profile->mem_latencies[curr->ind];
    int8_t l = max(0, l2 - l1);

    _update_latency_dict(&l, prev->inst, curr->inst, arr);
}

/* Calculate the number of cycles needed between
 * two instructions, based on when a vmem from
 * the prior instruction is ready for consumption
 * by a vmem from the current instruction. */
void build_latency_edge_vmem(VMem *prev, VMem *curr, InstInfo *arr) {
    InstExecutionProfile *prev_profile = get_inst_profile(prev->inst);
    InstExecutionProfile *curr_profile = get_inst_profile(curr->inst);

    int8_t l1 = prev_profile->reg_latencies[prev->ind];
    int8_t l2 = curr_profile->reg_latencies[curr->ind];
    int8_t l = max(0, l2 - l1);

    _update_latency_dict(&l, prev->inst, curr->inst, arr);
}

/* Build DAG for instructions in a block and
 * populate . */
// TODO: cheap alloc for array in dict values.
void buildDAG(Block *block, BlockInfo *block_info) {
    InstInfo *inst_info_arr = block_info->inst_info_arr.arr;
    size_t *inst_order = block_info->inst_order.arr;

    /* Mapping a vreg variable to its previous
     * uses. */
    Dict last_uses;
    /* Mapping a vreg variable to its previous
     * defs. */
    Dict last_defs;
    /* Mapping a vmem base variable to previous loads
     * that alias memories with the same base. */
    Dict last_loads;
    /* Mapping a vmem base variable to previous stores
     * that alias memories with the same base. */
    Dict last_stores;

    DLL insts = block->insts;

    /* TODO: Assume latency for each operand is populated
     * in Inst. */

    /* Build latency edges in forward direction
     * and calculate depth. */
    for (inst in insts) {
        /* Set up write-after-read and write-after-write
         * dependency for virtual registers. */
        if (write to a virtual register) {
            /* Build latency edges with all prior writes of the
             * same variable. Note that we cannot apply
             * transitive reduction here because latency for
             * A -> B -> C may be smaller than A -> C. */
            Array *defs = last_defs[inst->lhs_vreg.var];
            for (def in defs)
                build_latency_edge_vreg(def, inst->lhs_vreg, inst_info_arr);

            /* Build latency edges with all prior reads of the
             * same variable. */
            Array *uses = last_uses[inst->lhs_vreg.var];
            for (use in uses)
                build_latency_edge_vreg(use, inst->lhs_vreg, inst_info_arr);
        }

        /* Set up read-after-write dependency for virtual registers. */
        for (vreg used in inst) {
            VReg *defs = last_defs[vreg.var];
            for (def in defs)
                build_latency_edge_vreg(def, vreg, inst_info_arr);
            last_uses[vreg.var].add(vreg);
        }

        /* We add LHS operand to last_def after
         * setting up all the dependency edges
         * for virtual registers, so that an
         * instruction such as x = x + y
         * does not erroneously depend on itself. */
        if (there is LHS) last_defs[LHS->var].add(LHS);

        /* Set write-after-read or write-after-write
         * dependency for virtual memories. Scan previous
         * loads and stores, identify vmems that may
         * alias the stored vmem and set up latency edges. */
        if (store to vmem) {
            /* If a prior instruction stores to or loads from
             * any memory, set up latency edge with the instruction. */
            if (for str_vmem in last_stores[ALL_MEM]) {
                build_latency_edge_mem(str_vmem, vmem, inst_info_arr);
            }
            if (for ldr_vmem in last_loads[ALL_MEM]) {
                build_latency_edge_mem(ldr_vmem, vmem, inst_info_arr);
            }

            /* If storing to any abstract memory, set up
             * dependency edge with every prior vmem. */
            if (vmem points to ALL_MEM) {
                for (every key in last_stores) {
                    for (every value in last_stores[key])
                        build_latency_edge_vmem(value, vmem, inst_info_arr);
                }
                for (every key in last_loads) {
                    for (every value in last_loads[key])
                        build_latency_edge_vmem(value, vmem, inst_info_arr);
                }
                last_stores[ALL_MEM].add(vmem);
            }
            /* Else, find possible aliasing candidates
             * and set up dependency edge accordingly. */
            else {
                for (every base in vmem.pt_set) {
                    Array *str_vmems = last_stores[base];
                    for (str_vmem in str_vmems) {
                        if (alias(str_vmem, vmem))
                            build_latency_edge_vmem(str_vmem, vmem, inst_info_arr);
                    }
                    Array *ldr_vmems = last_loads[base];
                    for (ldr_vmem in ldr_vmems) {
                        if (alias(ldr_vmem, vmem))
                            build_latency_edge_vmem(ldr_vmem, vmem, inst_info_arr);
                    }
                }
                last_stores[base].add(vmem);
            }
        }

        /* Set read-after-write dependency for
         * virtual memories. Scan previous stores,
         * identify vmems that may alias the loaded
         * vmem and set up latency edges. */
        if (load from vmem) {
            /* If loading from any abstract memory, set up
             * dependency edge with every prior stored vmem. */
            if (vmem points to ALL_MEM) {
                for (every key in last_stores) {
                    for (every value in last_stores[key])
                        build_latency_edge_vmem(value, vmem, inst_info_arr);
                }
                last_loads[ALL_MEM].add(vmem);
            }
            /* Else, find possible aliasing candidates
             * and set up dependency edge accordingly. */
            else {
                for (every base in vmem.pt_set) {
                    Array *str_vmems = last_stores[base];
                    for (str_vmem in str_vmems) {
                        if (alias(str_vmem, vmem))
                            build_latency_edge_vmem(str_vmem, vmem, inst_info_arr);
                    }
                    last_loads[base].add(vmem);
                }
            }

            /* If a prior store instruction stores to any memory,
             * set up latency edge with the instruction. */
            if (for str_vmem in last_stores[ALL_MEM]) {
                build_latency_edge_vmem(str_vmem, vmem, inst_info_arr);
            }
        }

        /* Calculate depth of the current instruction. */
        InstInfo *inst_info = &inst_info_arr[inst_order[inst->ind]];
        Dict *latencies = &inst_info->pred_latencies;
        int depth = 0;
        for (int i = 0; i < latencies->cap; i++) {
            if (key) {
                InstInfo *pred = &inst_info_arr[inst_order[key]];
                depth = max(depth, latencies[key] + pred->depth);
            }
        }
        inst_info->depth = depth;

        /* Calculate resource demand of the current instruction. */
        InstExecutionProfile *profile = get_inst_profile(inst);
        for (int t = 0; t < profile->num_cycles; t++) {
            for (int r = 0; r < NUM_RESOURCE_TYPE; r++) {
                uint8_t units = profile->res_profile[t][r];
                inst_info->res_demand[r] += units;
                block_info->rrd[r] += units;
            }
        }

        /* Calculate outdegrees. */
        inst_info->outdegree = inst_info->succ_latencies.k_size;
    }
}

/* Calculate the incremental effect of each instruction
 * on register pressure in bottom-up scheduling. */
int calc_reg_pressure_diff(
    Inst *inst, InstInfo *arr, size_t *inst_order, Slot **inst_to_slot
) {
    // TODO: return if cached
    int pressure = 0;

    Slot *next_slot = inst_to_slot[inst->ind]->next;

    /* If LHS variable is live after the instruction, decrease
     * reg pressure by 1 unit. */
    if (inst has vreg lhs) {
        if (bool_bitarray(lhs.var->ind, next_slot->live)
            lhs live after inst) pressure--;
    }
    /* For each distinct RHS variable used, if the variable
     * is dead after the instruction, increase
     * reg pressure by 1 unit. */
    size_t vars_assessed[MAX_OPERAND];
    for (vreg used) {
        bool dup = false;
        for (int i = 0; i < MAX_OPERAND; i++) {
            if (vars_assessed[i] == vreg.var->ind) {
                dup = true;
                break;
            }
        }
        if (!dup && !bool_bitarray(vreg.var->ind, next_slot->live))
            pressure++;
    }
    arr[inst_order[inst->ind]].rp_diff = pressure;

    return pressure;
}

uint8_t calc_critical_resource(int *rrd) {
    /* TODO - cache */
    uint8_t r = NUM_RESOURCE_TYPE + 1;  // represents NULL resource
    int demand = 0;
    for (uint8_t i = 0; i < NUM_RESOURCE_TYPE; i++) {
        int t = rrd[i] / available_resources[i];
        if (t > demand) {
            demand = t;
            r = i;
        }
    }
    return r;
}

Inst *select_default(
    Inst *inst1,
    Inst *inst2,
    InstInfo *arr,
    int *rrd,
    size_t *inst_order
) {
    /* TODO - same as select with reg pressure,
     * just no reg pressure. */
}

Inst *select_with_reg_pressure(
    Inst *inst1,
    Inst *inst2,
    InstInfo *arr,
    int *rrd,
    size_t *inst_order,
    Slot **inst_to_slot
) {
    if (!inst1) return inst2;
    if (!inst2) return inst1;

    /* Return the instruction that results in less
     * incremental reg pressure. */
    int p1 = calc_reg_pressure_diff(inst1, arr, inst_order, inst_to_slot);
    int p2 = calc_reg_pressure_diff(inst2, arr, inst_order, inst_to_slot);
    if (p1 < p2) return inst1;
    if (p1 > p2) return inst2;

    InstInfo *info1 = &arr[inst_order[inst1->ind]];
    InstInfo *info2 = &arr[inst_order[inst2->ind]];

    /* Return the instruction with greater depth. */
    if (info1->depth > info2->depth) return inst1;
    if (info1->depth < info2->depth) return inst2;

    /* Return the instruction with more predecessors. */
    if (info1->pred_latencies.k_size > info2->pred_latencies.k_size) return inst1;
    if (info1->pred_latencies.k_size < info2->pred_latencies.k_size) return inst2;

    /* Return the instruction that uses more critical resources. */
    int cr = calc_critical_resource(rrd);
    if (info1->res_demand[cr] > info2->res_demand[cr]) return inst1;
    if (info1->res_demand[cr] < info2->res_demand[cr]) return inst2;

    /* Return the latter instruction by original order. */
    if (inst_to_slot[inst1->ind]->ind > inst_to_slot[inst2->ind]->ind) return inst1;
    return inst2;
}

/* Schedule instructions backwards. Choose the next
 * instruction to schedule among instructions with
 * outdegree = 0. Identify the "latest" time to
 * schedule the instruction without resource constraint.
 *
 * To constrain the size of the resource table,
 * we use a circular buffer and a defined lookback window.
 * We define a buffer size >= the lookback window + 
 * the maximum number of time steps an instruction need.
 * For each instruction, we take max(initial candidate
 * scheduled time, latest scheduled time - lookback) as
 * the final candidate scheduled time. The initial
 * candidate scheduled time is calculated by taking the
 * max(predecessor scheduled time + latency). */
void listScheduling(
    Block *block,
    bool pre_alloc,
    BlockInfo *block_info,
    RegAllocContext *c
) {
    if (pre_alloc && !c) raise error;

    /* Worklist of instructions ready to schedule. */
    DLL worklist;

    /* Push instructions with 0 successor to worklist. */
    DLL insts = block->insts;
    for (each inst backwards) {
        if (!inst_info[inst->ind].outdegree)
            push_dll_elem(inst, &queue);
    }

    int end_time, start_time, frontier, reg_pressure;

    /* Frontier represents the earliest cycle where
     * no instructions are scheduled. Since we are scheduling
     * backwards, it represents the "greatest" start time
     * among all instructions + 1. */
    frontier = 0;

    /* Set reg pressure to the cardinality of the live out set. */
    if (pre_alloc) {
        DLL slots = ((DLL *) c->block_to_slots.arr)[block->ind];
        reg_pressure = count_ones_bitarray(slots.tail->live);
    }

    while (worklist.size) {
        Inst *inst = NULL;

        /* Select next instruction to schedule. If scheduling
         * is executed prior to reg allocation, allow selection
         * heuristics to be register aware. */
        if (pre_alloc && reg_pressure > REG_THRESHOLD) {
            for (candidate in worklist) {
                inst = select_with_reg_pressure(
                    inst,
                    candidate,
                    block_info->inst_info_arr.arr,
                    block_info->rrd,
                    block_info->inst_order.arr,
                    c->inst_to_slot.arr
                );
            }
        }
        else {
            for (candidate in worklist) {
                inst = select_default(
                    inst,
                    candidate,
                    block_info->inst_info_arr.arr,
                    block_info->rrd,
                    block_info->inst_order.arr
                );
            }
        }

        /* Remove instruction from the candidate pool. */
        remove_dll_elem(inst, &worklist);

        size_t local_ind = ((size_t *) block_info->inst_order.arr)[inst->ind];
        InstInfo *inst_info = &((InstInfo *) block_info->inst_info_arr.arr)[local_ind];

        /* Find the next candidate start time. Since we are
         * scheduling backwards, the next candidate start time
         * is max(scheduled time of successor + latency) among
         * all successors. */
        for (key, latency in inst_info->succ_latencies) {
            get succ local ind of key;
            get succ inst info of key;
            int t = succ_inst_info->schedule + latency;
            start_time = t > start_time ? t : start_time;
        }
        /* Find the next candidate end time. End time cannot
         * be "smaller" than frontier minus RESOURCE_LOOKBACK
         * due to the circular buffer. Adjust start time
         * accordingly. */
        InstExecutionProfile *profile = get_inst_profile(inst);
        end_time = start_time - profile->num_cycles;
        end_time = max(end_time, frontier - RESOURCE_LOOKBACK);
        start_time = end_time + profile->num_cycles;

        /* Attempt to schedule the instruction at the
         * candidate time. If there is resource conflict,
         * increment the candidate time until a schedule
         * can be found. */
        while (true) {
            bool schedule_found = true;
            for (time_step needed in instruction backwards) {
                for resource needed at each instruction time_step {
                    int units = inst_info.resource_demand[time_step][resource];
                    int buffer_time = (end_time + cycles_needed - time_step) % BUFFER_SIZE;
                    int occupied = buffer_time within lookback ? rt[buffer_time][resource] : 0;
                    if (resource_per_cycle[resource] - occupied < units) {
                        schedule_found = false;
                        break;
                    }
                }
                if (!schedule_found) break;
            }
            if (!schedule_found) {
                end_time++;
                start_time++;
                continue;
            }
            break;
        }

        /* Populate resource buffer. */
        for (time_step needed in instruction backwards) {
            for resource needed at each instruction time_step {
                int units = inst_info.resource_demand[time_step][resource];
                int buffer_time = (end_time + cycles_needed - time_step) % BUFFER_SIZE;
                int occupied = buffer_time within lookback ? rt[buffer_time][resource] : 0;
                rt[buffer_time][resource] = occupied + units;
            }
        }

        /* Update the frontier. */
        frontier = max(frontier, start_time + 1);

        /* Schedule the current instruction. */
        inst_info->schedule = start_time;

        /* Push eligible unscheduled instructions
         * to priority queue. */
        for (predecessors of inst) {
            int ind = predecessor->slot->ind;
            outdegree[ind]--;
            if (!outdegree[ind]) queue.push(predecessor);
        }

        /* Update register pressure trackers. */
        reg_pressure += inst_info->rp_diff;

        /* Update liveness information before the
         * current instruction. */
        if (inst is def) mark LHS dead at slot before inst;
        for (each vreg used) mark vreg live at slot before inst;

        /* Update resource demand trackers. */
        for (int i = 0; i < NUM_RESOURCE_TYPE; i++) {
            block_info->rrd[resource] -= inst_info.resource_demand[resource];
            /* TODO: invalidate cache. */
        }
    }

    /* Re-order instructions in a post-pass. */
    sort_dll(insts, func_to_reverse_sort_by_schedule);
    if (pre_alloc) {
        // TODO: sort slots and apply ind to slots
    }
}
