/* Instruction scheduling.
 * Pre-allocation scheduling takes register pressure into
 * account. Post-allocation scheduling doesn't. */
#include "bitarray.h"
#include "dll.h"
#include "vreg.h"
#include "vmem.h"
#include "block.h"
#include "instprofile.h"
#include "liveness.h"
#include "scheduling.h"

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

    for (int i = 0; i < NUM_RESOURCE_TYPE; i++)
        block_info->rrd[i] = 0;

    for (int i = 0; i < BUFFER_SIZE; i++) {
        for (int j = 0; j < NUM_RESOURCE_TYPE; j++)
            block_info->rt[i][j] = 0;
    }
}

void free_block_info(BlockInfo *block_info) {
    // free arrays

    free(block_info);
}

void scheduling(Array *blocks, bool pre_alloc, LivenessDFAObject *objs) {
    /* Liveness analysis must be run before pre-allocation scheduling. */
    if (pre_alloc && !objs) raise error;

    BlockInfo *block_info = init_block_info(pre_alloc);

    for (int i = 0; i < blocks->size; i++) {
        Block *block = blocks[i];
        reset_block_info(block, block_info);

        /* Populate inst_order and initialize inst_info. */
        size_t counter = 0;
        for (each inst) {
            block_info->inst_order[inst->ind] = counter;
            InstInfo *info = block_info->inst_info_arr[counter];
            info->inst = inst;
            counter++;
        }

        /* Build DAG and record metadata. */
        buildDAG(block, block_info);

        /* Schedule instructions. */
        listScheduling(block, pre_alloc, block_info, objs);
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

/* Estimate the incremental effect of each instruction
 * on register pressure in bottom-up scheduling.
 * We estimate this metric by calculating liveness
 * based on all scheduled instructions thus far,
 * and calculate the incremental reg diff based
 * on this liveness. Note that, this is only an
 * estimation because an instruction could technically
 * be scheduled behind frontier in the lookback window,
 * in which case the "current liveness" may not be the
 * real liveness. However, this is a small edge case
 * and we can take the tradeoff here.
 * Intuition:
 * Assume A is at the frontier, and we want to schedule B.
 * B does not conflict with A, and we can schedule B
 * behind A. Suppose set IN represents the liveness state
 * behind A when B has not been scheduled yet.
 * When we calculate reg pressure of B, we estimate the
 * current liveness to be useA U (IN - defA), whereas
 * when we schedule B, the liveness at the slot after B
 * is actually just IN. Since B cannot conflict with A,
 * useA does not intersect with defB, and useB does not
 * intersect with defA. Therefore, the liveness of the LHS
 * of B would be the same for both IN and useA U (IN - defA).
 * On the other hand, the liveness of the RHS of B may
 * not be the same. For example, suppose RHS is dead at IN.
 * if useA contains the RHS, it would be live at
 * useA U (IN - defA). */
int calc_reg_pressure_diff(
    Inst *inst, InstInfo *arr, size_t *inst_order, BitArray *curr_live
) {
    int pressure = 0;

    /* If LHS variable is live after the instruction, decrease
     * reg pressure by 1 unit. */
    if (inst has vreg lhs && bool_bitarray(lhs.var->ind, curr_live))
        pressure--;

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
        if (!dup && !bool_bitarray(vreg.var->ind, curr_live))
            pressure++;
    }
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
    BitArray *curr_live
) {
    if (!inst1) return inst2;
    if (!inst2) return inst1;

    /* Return the instruction that results in less
     * incremental reg pressure. */
    int p1 = calc_reg_pressure_diff(inst1, arr, inst_order, curr_live);
    int p2 = calc_reg_pressure_diff(inst2, arr, inst_order, curr_live);
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
    if (inst_order[inst1->ind] > inst_order[inst2->ind]) return inst1;
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
    LivenessDFAObject *objs
) {
    if (pre_alloc && !c) raise error;

    size_t *inst_order = (size_t *) block_info->inst_order.arr;
    InstInfo *inst_info_arr = (InstInfo *) block_info->inst_info_arr.arr;

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

    /* Curr_live represents the liveness state at the frontier.
     * Only relevant for register-aware scheduling. */
    BitArray *curr_live;
    BitArray *tmp_use;
    BitArray *tmp_def;

    /* Inititalize curr_live to be the live out set.
     * Set reg pressure to the cardinality of curr_live. */
    if (pre_alloc) {
        curr_live = objs[block->ind].out;
        reg_pressure = count_ones_bitarray(curr_live);

        tmp_use = bitarray();
        tmp_def = bitarray();
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
                    inst_info_arr,
                    block_info->rrd,
                    inst_order,
                    curr_live
                );
            }
        }
        else {
            for (candidate in worklist) {
                inst = select_default(
                    inst,
                    candidate,
                    inst_info_arr,
                    block_info->rrd,
                    inst_order
                );
            }
        }

        /* Remove instruction from the candidate pool. */
        remove_dll_elem(inst, &worklist);

        size_t local_ind = inst_order[inst->ind];
        InstInfo *inst_info = &inst_info_arr[local_ind];

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
        end_time = start_time - profile->num_cycles + 1;
        end_time = max(end_time, frontier - RESOURCE_LOOKBACK);
        start_time = end_time + profile->num_cycles - 1;

        /* Attempt to schedule the instruction at the
         * candidate time. If there is resource conflict,
         * increment the candidate time until a schedule
         * can be found. */
        // TODO: test that exec profile and available resources
        // are possible. E.g., if an inst requires 3 units
        // of MEM in a cycle, available_resources[MEM] >= 3.
        while (true) {
            bool schedule_found = true;
            for (int t = profile->num_cycles - 1; t >= 0; t--) {
                uint8_t rt_row = (start_time - t) % BUFFER_SIZE;
                uint8_t lookback_start = (frontier - RESOURCE_LOOKBACK) % BUFFER_SIZE;
                uint8_t lookback_end = frontier % BUFFER_SIZE;

                for (uint8_t r = 0; r < NUM_RESOURCE_TYPE; r++) {
                    int units = profile->res_profile[t][r];
                    int occupied;
                    /* If the target rt_row belongs to the lookback window,
                     * identify the current occupied resources. Otherwise,
                     * assume all resources are unoccupied. */
                    if (rt_row >= lookback_start && rt_row <= lookback_end)
                        occupied = block_info->rt[rt_row][r];
                    else occupied = 0;
                    /* If resource conflict exists, abort scheduling. */
                    if (available_resources[r] - occupied < units) {
                        schedule_found = false;
                        break;
                    }
                }
                if (!schedule_found) break;
            }
            /* If scheduling failed, increment candidate time and try again. */
            if (!schedule_found) {
                end_time++;
                start_time++;
                continue;
            }
            break;
        }

        /* Populate resource buffer. */
        for (int t = profile->num_cycles - 1; t >= 0; t--) {
            uint8_t rt_row = (start_time - t) % BUFFER_SIZE;
            uint8_t lookback_start = (frontier - RESOURCE_LOOKBACK) % BUFFER_SIZE;
            uint8_t lookback_end = frontier % BUFFER_SIZE;

            for (uint8_t r = 0; r < NUM_RESOURCE_TYPE; r++) {
                int units = profile->res_profile[t][r];
                int occupied;
                /* If the target rt_row belongs to the lookback window,
                    * identify the current occupied resources. Otherwise,
                    * assume all resources are unoccupied. */
                if (rt_row >= lookback_start && rt_row <= lookback_end)
                    occupied = block_info->rt[rt_row][r];
                else occupied = 0;

                block_info->rt[rt_row][r] = occupied + units;
            }
        }

        /* Update the frontier. */
        frontier = max(frontier, start_time + 1);

        /* Schedule the current instruction. */
        inst_info->schedule = start_time;

        /* Push eligible unscheduled instructions
         * to priority queue. */
        for (key in inst_info->pred_latencies) {
            InstInfo *pred_info = &inst_info_arr[inst_order[key]];
            pred_info->outdegree--;
            if (!pred_info->outdegree)
                insert_ddl_elem(pred_info->inst, NULL, &worklist);
        }

        /* Update curr_live. Estimate reg_pressure as
         * the number of live variables in curr_live.
         * We update curr_live based on the use
         * and def set of the current scheduled instruction.
         * This leads to an accurate calculation of the
         * actual liveness state at the frontier, despite
         * that the order for the incremental calculation
         * may be different from the actual order of
         * scheduled instructions.
         * Intuition:
         * Suppose the liveness calculation order places
         * A before B, and the actual order of instructions
         * places B before A. This can only happen if A, B
         * don't conflict each other. Therefore, useA and
         * defB don't intersect, and useB and defA don't
         * intersect. Note the following cases:
         * Case 1: curr_live = useA U (useB - defA) U (IN - defA U defB)
         * Case 2: curr_live = useB U (useA - defB) U (IN - defA U defB)
         * In both cases, curr_live is identical. */
        if (pre_alloc) {
            set_use_vreg(inst, tmp_use);
            set_def_vreg(inst, tmp_def);
            curr_live = tmp_use U (curr_live - tmp_def);

            reset_bitarray(tmp_def);
            reset_bitarray(tmp_use);

            /* Update register pressure trackers. */
            reg_pressure = count_ones_bitarray(curr_live);
        }

        /* Update resource demand trackers. */
        for (int r = 0; r < NUM_RESOURCE_TYPE; r++) {
            block_info->rrd[r] -= inst_info->res_demand[r];
        }
    }

    /* Re-order instructions in a post-pass. */
    sort_dll(insts, func_to_reverse_sort_by_schedule);

cleanup:
    if (pre_alloc) {
        free_bitarray(tmp_def);
        free_bitarray(tmp_use);
    }
}
