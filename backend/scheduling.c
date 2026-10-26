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

        ins = ins->next;
    }
}

int priority_function(MInst *inst) {
    /* TODO: add consideration for resource constraint */
    return inst->height;
}

/* 
    TODO:
    Prioritize instruction that terminate a live range,
    and deprioritize instruction that start a live range.
 */
int rp_aware_priority_function(MInst *inst) {
    /* TODO */
}

int rt[BUFFER_SIZE][RESOURCES];  // Circular buffer

/* TODO: Criteria for priority */
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
    size_t h_index = block->head->index;
    size_t t_index = block->tail->index;
    inst_size = t_index - h_index + 1;
 
    Array<int> schedule(inst_size);
    Array<int> indegree(inst_size);
    Array<Array<MInst *>> r_schedule(inst_size * MAX_TIME_STEP);  // TODO: refine?

    PriorityQueue<MInst *> pq;  // Normal queue
    PriorityQueue<MInst *> int_ra_pq;  // Register aware queue
    PriorityQueue<MInst *> fp_ra_pq;

    /* Populate indegrees and push instructions
     * without predecessors to priority queue. */
    MInst *inst = block->head;
    while (inst) {
        int ind = inst->index - h_index;
        indegree[ind] = inst->predecessors.size();
        if (!indegree[ind]) pq.push(inst);
        inst = inst->next;
    }

    int candidate;
    int scheduled;
    int unscheduled = 0;  // earliest unscheduled time
    while (!pq.empty()) {
        MInst *top = NULL;
        /* Prioritize register-aware heuristics if
         * register pressure is high. */
        if (curr_irp > INT_RP_THRESHOLD) {
            while (!ra_pq.empty()) {
                top = ra_pq.front();
                if (schedule[top->index - h_index]) continue;
                break;
            }
        }

        else if (curr_frp > FP_RP_THRESHOLD) {
            same as above but for FP;
        }

        else {
            top = priority.front();
            pq.pop();
            if (schedule[top->index - h_index]) continue;
        }

        /* Find the next scheduled time. */
        candidate = max(predecessor scheduled time + latency);
        scheduled = max(candidate, unscheduled - RESOURCE_LOOKBACK);
        while (true) {
            bool schedule_found = true;
            for (time_step, resource, unit needed in instruction) {
                int buffer_row = (scheduled + time_step) % BUFFER_SIZE;
                if (rt[buffer_row][resource] remaining < unit) {
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
            rt[buffer_row][resource] += unit;
        }

        /* Update the earliest unscheduled time. */
        unscheduled = max(unscheduled, scheduled + total time step for instruction);

        /* Schedule the current instruction. */
        schedule[top->index - h_index] = scheduled;
        r_schedule[scheduled].push(top);

        /* Push eligible unscheduled instructions
         * to priority queue. */
        for (successor : top) {
            int ind = successor->index - h_index;
            indegree[ind]--;
            if (!indegree[ind]) pq.push(successor);
        }
    }

    /* TODO: 
    * Move forward (hoisting):
    * if def, range carrying that def extend forward
    * if last use, range carrying that use shortens forward
    * Move backward (sinking):
    * if def, range carrying that def shortens backward
    * if last use, range carrying that use extends backward
    */
    for (instruction array in r_scheduled) {
        for (instruction in instruction array) {
            set up prev and next;
            update indices;
            update live ranges;
        }
    }
}
