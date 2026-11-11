/* TODO: 2nd round scheduling does not require
 * updating live intervals or being register aware.

 * Do not rely on use def chain since it won't be preserved
 * after reg alloc.

 * Need to run live variable analysis before 1st round
 * scheduling. Build intervals after 1st round scheduling.
 * 
 * Set up slots before scheduling. Each instruction is tied
 * to a slot, and each slot ties back to an instruction.
 */

/* Lookback window for list scheduling
 * in a circular buffer. */
#define RESOURCE_LOOKBACK 10
#define BUFFER_SIZE 30

void scheduling(Array<MBlock *> blocks) {
    for (block in blocks) {
        buildDAG(block);
        listScheduling(insts);
    }
}

int remaining_resource_demand[RESOURCES];
int resource_per_cycle[RESOURCES];
int rt[BUFFER_SIZE][RESOURCES];  // Circular buffer

/* Calculate latency between instructions.
 * Latency is operand or memory dependent.
 * Latency represents the number of cycles between
 * top of instruction A and top of instruction B. */
void build_latency_edge(VReg *prev, VReg *curr) {
    int l1 = curr->inst->lhs_latency;
    int l2 = prev->inst->operand_or_memory_latency;
    int l = max(0, l2 - l1);

    /* TODO - edge between two instructions may
     * appear twice with different latencies. */
    curr_inst_info.predecessors.add(prev->inst);
    curr_inst_info.p_latencies.add(l);
    prev_inst_info.successors.add(curr->inst);
    prev_inst_info.s_latencies.add(l);
}

/* Build DAG for instructions in a block. */
void buildDAG(Block *block) {
    /* defs to same reg in order
     * defs before uses
     * uses before redefs
     * writes to aliased memory in order
     * writes before reads to aliased memory
     * reads before rewrites to aliased memory
     * Each edge has a latency metric, marking
     * minimal number of cycles needed for dependent
     * to wait after predecessor.
     * */
    Dict<Var *, Array<VReg *>> last_uses;
    Dict<Var *, VReg *> last_def;
    VMem *last_memory = NULL;
    MInst *inst = block->head;

    /* TODO: Assume latency for each operand is populated
     * in MInst. */

    /* Do this forward to calculate depth.
     * If A -> B -> C, no need for A -> C, so we can
     * use last_uses and last_def to set up the
     * dependency chain. */
    while (inst) {
        /* Set up write-after-read and write-after-write
         * dependency in case of virtual registers. */
        if (write to a virtual register) {
            VReg *last = last_def[LHS->var];
            build_latency_edge(last, LHS);

            /* All prior uses need to be accounted for,
             * because uses don't have dependencies with each other. */
            for (use in last_uses[LHS->var]) {
                build_latency_edge(use, LHS);
            }

            /* Clear last uses for LHS var. The next
             * def would need to set up edge with this
             * def, so existing last uses for LHS var
             * would have already been accounted for. */
            last_uses[LHS->var].clear();
        }

        /* Set up read-after-write dependency
         * in case of virtual registers. */
        for (vreg used in ins) {
            VReg *last = last_def[vreg->var];
            build_latency_edge(last, vreg);
            last_uses[vreg->var].add(vreg);
        }

        /* We add LHS operand to last_def after
         * setting up all the dependency edges
         * for virtual registers, so that an
         * instruction such as x = x + y
         * does not erroneously depend on itself. */
        if (there is LHS) last_def[LHS->var] = LHS;

        /* Ensure all memory instructions are executed
         * in order. We take this conservative measure
         * rather than assessing aliases precisely
         * for a lightweight scheduling solution. */
        if (memory) {
            build_latency_edge(last_memory, memory);
            last_memory = memory;
        }

        /* Calculate depth of the current instruction. */
        int depth = 0;
        for (int i = 0; i < inst_info.predecessors.size(); i++) {
            int h = inst_info.predecessors[i]->depth;
            int l = inst_info.p_latencies[i]->latency;
            depth = max(depth, h + l);
        }
        inst_info.depth = depth;

        /* Calculate resource need of the current instruction. */
        ins_info.cycle = total cycles needed;
        for (cycle needed) {
            for resource {
                ins_info.resource_demand[resource] += time_step[cycle][resource];
                remaining_resource_demand[resource] += time_step[cycle][resource];
            }
        }

        inst = inst->next;
    }
}

int calc_reg_pressure_diff(MInst *inst) {
    /* TODO - cache */

    int pressure = 0;
    if (inst is def and lhs live after inst) pressure--;
    for (vregs used) {
        if (vreg is dead after inst) pressure++;
    }
    inst_info.reg_pressure = pressure;

    return pressure;
}

int calc_critical_resource() {
    /* TODO - cache */
    int r = -1;  // Represent NULL resource somehow
    int demand = 0;
    for resource in resources {
        int t = remaining_resource_demand[resource] / resource_each_cycle[resource];
        if (t > demand) {
            demand = t;
            r = resource;
        }
    }
    return r;
}

MInst *select_default(MInst *inst1, MInst *inst2) {
    /* TODO - same as select with reg pressure,
     * just deprioritize reg pressure. */
}

MInst *select_with_reg_pressure(MInst *inst1, MInst *inst2) {
    if (!inst1) return inst2;
    if (!inst2) return inst1;

    /* Return the instruction that results in less
     * incremental reg pressure. */
    int p1 = calc_reg_pressure_diff(inst1);
    int p2 = calc_reg_pressure_diff(inst2);
    if (p1 < p2) return inst1;
    if (p1 > p2) return inst2;

    /* Return the instruction with greater depth. */
    if (a.depth > b.depth) return inst1;
    if (a.depth < b.depth) return inst2;

    /* Return the instruction with more predecessors. */
    if (a.predecessors.size() > b.predecessors.size()) return inst1;
    if (a.predecessors.size() < b.predecessors.size()) return inst2;

    /* Return the instruction that uses more critical resources. */
    int critical_resource = calc_critical_resource();
    if (a.resource_demand[critical_resource] > b.resource_demand[critical_resource]) return inst1;
    if (b.resource_demand[critical_resource] > a.resource_demand[critical_resource]) return inst2;

    /* Return the latter instruction by original order. */
    if (inst1->slot->ind > inst2->slot->ind) return inst1;
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
void listScheduling(MBlock *block) {
    size_t inst_size = get # of insts in block;
 
    Array<int> schedule(inst_size);
    Array<int> outdegree(inst_size);

    DLL<MInst *> queue;  // Ready queue

    /* Populate outdegrees and push instructions
     * without successors to ready queue. */
    MInst *inst = block->tail;
    while (inst) {
        int ind = inst->slot->ind;
        outdegree[ind] = inst->successors.size();
        if (!outdegree[ind]) queue.push(inst);
        inst = inst->prev;
    }

    int end_time;
    int start_time;
    int frontier = 0;  // earliest scheduled time
    int reg_pressure = # of live_out vregs;

    while (!queue.empty()) {

        MInst *inst = NULL;

        /* Select next instruction to schedule. */
        if (reg_pressure > threshold) {
            for instruction in queue {
                inst = select_with_reg_pressure(inst, instruction);
            }
        }
        else {
            for instruction in queue {
                inst = select_default(inst, instruction);
            }
        }

        ddl_remove(inst, queue);

        /* Find the next candidate scheduled time. */
        int start_time = 0;
        for (s in successors) {
            int t = schedule[s] + inst_info.latencies[s];
            start_time = t > start_time ? t : start_time;
        }
        end_time = start_time - inst->cycles_needed;
        end_time = max(end_time, frontier - RESOURCE_LOOKBACK);
        start_time = end_time + inst->cycles_needed;

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
        frontier = max(frontier, start_time);

        /* Schedule the current instruction. */
        schedule[inst->slot->ind] = start_time;

        /* Push eligible unscheduled instructions
         * to priority queue. */
        for (predecessors of inst) {
            int ind = predecessor->slot->ind;
            outdegree[ind]--;
            if (!outdegree[ind]) queue.push(predecessor);
        }

        /* Update register pressure trackers. */
        reg_pressure += calc_reg_pressure_diff(inst);

        /* Update liveness information before the
         * current instruction. */
        if (inst is def) mark LHS dead at slot before inst;
        for (each vreg used) mark vreg live at slot before inst;

        /* Update resource demand trackers. */
        for resource in resources {
            remaining_resource_demand[resource] -= inst_info.resource_demand[resource];
            /* TODO: invalidate cache. */
        }
    }

    /* Order instructions and slots in a post pass.
     * TODO - separate out slot pass if 2nd round doesn't need it. */
    sort schedule in reverse order;

    MInst *prev = NULL;
    Slot *prev_slot = NULL;
    size_t ind = 0;
    for (inst in schedule) {
        inst->prev = prev;
        if (prev) prev->next = inst;
        slot->prev = prev_slot;
        if (prev_slot) prev_slot->next = slot;
        slot->ind = ind++;
    }

    slot->next = block_info->end_slot;
    block_info->end_slot->prev = slot;
    block_info->end_slot->ind = ind;
}
