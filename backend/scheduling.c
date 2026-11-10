/* TODO: 2nd round scheduling does not require
 * updating live intervals or being register aware. */

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
    Dict<Variable *, VReg *> last_occurrence;
    VMem *last_memory = NULL;
    MInst *ins = block->head;

    /* TODO: Assume latency for each operand is populated
     * in MInst. */

    /* TODO - do this backwards to calculate height. */
    while (ins && ins->block == block) {
        /* Set up write-after-read and write-after-write
         * dependency in case of virtual registers. */
        if (write to a virtual register) {
            VReg *written = ins->lhs;
            VReg *p = last_occurrence[p->var];

            if (p->inst->block == ins->block) {
                /* Calculate latency between instructions.
                 * Latency is operand-dependent. */
                int l1 = ins->lhs_latency;
                int l2 = p->inst->operand_latency;
                int l = max(0, l2 - l1);

                ins->predecessors.add(p->inst);
                ins->latencies.add(l);
                p->inst->successors.add(ins);
            }
        }

        /* Set up read-after-write dependency
         * in case of virtual registers. */
        for (operand used in ins) {
            for (def in operand->defs) {
                if (def->inst->block == ins->block) {
                    /* Calculate latency between instructions.
                    * Latency is operand-dependent. */
                    int l1 = ins->operand_latency;
                    int l2 = def->inst->lhs_latency;
                    int l = max(0, l2 - l1);

                    ins->predecessors.add(def);
                    ins->latencies.add(l);
                    def->inst->successors.add(ins);
                }
            }
            last_occurrence[operand->name] = operand;
        }

        /* We add operand to last_occurrence after
         * setting up all the dependency edges
         * for virtual registers, so that an
         * instruction such as x = x + y
         * does not erroneously depend on itself. */
        last_occurrence[written->name] = written;

        /* Ensure all memory instructions are executed
         * in order. We take this conservative measure
         * rather than assessing aliases precisely
         * for a lightweight scheduling solution. */
        if (last_memory) {
            /* Calculate latency between instructions.
            * Latency is operand-dependent. */
            int l1 = ins->memory_latency;
            int l2 = last_memory->ins->memory_latency;
            int l = max(0, l2 - l1);

            ins->predecessors.add(last_memory->inst);
            ins->latencies.add(l);
            last_memory->inst->successors.add(ins);
        }
        last_memory = memory;

        /* Calculate height of the current instruction. */
        int height = 0;
        for (int i = 0; i < ins->predecessors.size(); i++) {
            int h = ins->predecessors[i]->height;
            int l = ins->latencies[i]->latency;
            height = max(height, h + l);
        }
        ins->height = height;

        /* TODO: Calculate resource need. */
        ins_info.cycle = total cycles needed;
        for (cycle needed) {
            for resource {
                ins_info.resource_demand[resource] += time_step[cycle][resource];
                remaining_resource_demand[resource] += time_step[cycle][resource];
            }
        }

        ins = ins->next;
    }
}

int calc_reg_pressure_diff(MInst *inst) {
    /* TODO - cache */

    int pressure = 0;
    if (inst is def and inst->slot->next marks beginning of a new interval) pressure++;
    for (vregs used) {
        if (vreg is live at inst->slot and vreg interval has 1 remaining use and vreg interval is not live out) pressure--;
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

    /* Return the instruction with greater height. */
    if (a.height > b.height) return inst1;
    if (a.height < b.height) return inst2;

    /* Return the instruction with more successors. */
    if (a.num_successors > b.num_successors) return inst1;
    if (a.num_successors < b.num_successors) return inst2;

    /* Return the instruction that uses more critical resources. */
    int critical_resource = calc_critical_resource();
    if (a.resource_demand[critical_resource] > b.resource_demand[critical_resource]) return inst1;
    if (b.resource_demand[critical_resource] > a.resource_demand[critical_resource]) return inst2;

    /* Return the first instruction by original order. */
    if (inst1->slot->ind < inst2->slot->ind) return inst1;
    return inst2;
}

/* To constrain the size of the resource table,
 * we use a circular buffer and a defined lookback window.
 * We define a buffer size >= the lookback window + 
 * the maximum number of time steps an instruction need.
 * For each instruction, we take max(initial candidate
 * scheduled time, latest scheduled time - lookback) as
 * the final candidate scheduled time. The initial
 * candidate scheduled time is calculated by taking the
 * max(predecessor scheduled time + latency). */
void listScheduling(MBlock *block) {
    size_t inst_size = block_info->end->ind;
 
    Array<int> schedule(inst_size);
    Array<int> indegree(inst_size);

    DLL<MInst *> queue;  // Ready queue

    /* Populate indegrees and push instructions
     * without predecessors to ready queue. */
    MInst *inst = block->head;
    while (inst) {
        int ind = inst->slot->ind;
        indegree[ind] = inst->predecessors.size();
        if (!indegree[ind]) queue.push(inst);
        inst = inst->next;
    }

    int candidate;
    int scheduled;
    int unscheduled = 0;  // earliest unscheduled time
    int reg_pressure = # of live_in vregs;

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

        /* Find the next scheduled time. */
        candidate = max(predecessor scheduled time + latency);
        scheduled = max(candidate, unscheduled - RESOURCE_LOOKBACK);
        while (true) {
            bool schedule_found = true;
            for (time_step, resource, unit needed in instruction) {
                int buffer_row = (scheduled + time_step) % BUFFER_SIZE;
                int occupied = before unscheduled ? rt[buffer_row][resource] : 0;
                if (resource_per_cycle[resource] - occupied < unit) {
                    schedule_found = false;
                    break;
                }
            }
            if (!schedule_found) {
                scheduled++;
                continue;
            }
        }

        /* Populate resource buffer. */
        for (time_step, resource, unit needed in instruction) {
            int buffer_row = (scheduled + time_step) % BUFFER_SIZE;
            int occupied = before unscheduled ? rt[buffer_row][resource] : 0;
            rt[buffer_row][resource] = occupied + unit;
        }

        /* Update the earliest unscheduled time. */
        unscheduled = max(unscheduled, scheduled + total time step for instruction);

        /* Schedule the current instruction. */
        schedule[top->slot->ind] = scheduled;

        /* Push eligible unscheduled instructions
         * to priority queue. */
        for (successor : top) {
            int ind = successor->slot->ind;
            indegree[ind]--;
            if (!indegree[ind]) queue.push(successor);
        }

        /* Update register pressure trackers. */
        reg_pressure += calc_reg_pressure_diff(top);
        for (each vreg interval at slot) {
            interval.remaining_use--;
            /* TODO: invalidate cache. */
        }

        /* Update resource demand trackers. */
        for resource in resources {
            remaining_resource_demand[resource] -= inst_info.resource_demand[resource];
            /* TODO: invalidate cache. */
        }
    }

    /* Update live intervals and data structures
     * based on the new scheduled order. */
    sort schedule by time step mapped to each instruction;

    if first instruction in new order different from the old one {
        for all vregs live_in {
            take the interval that starts with original start slot;
            mark the new first instruction slot as interval start;
        }
    }

    /* TODO - tag last use in a new pass? */

    for (instruction in schedule) {
        set up linkage with previous slot and inst;

        renumber slot;

        if (instruction defines LHS) {
            if slot->next (note: original) defines interval start {
                mark new next slot as start of that interval;
            }
        }

        for (each RHS used) {
            /* TODO - if not last use, but old interval ends, need to extend interval. 
             * ... */
        }

        /* TODO - update uses, defs */
    }

    renumber last slot;
}
