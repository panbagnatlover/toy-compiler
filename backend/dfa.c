
/* Separate object because data flow objects
 * are temporary and do not get reused. */
typedef struct DataFlowObject {
    BitArray in;
    BitArray out;
    BitArray use;
    BitArray def;
    Block *block;
    TFunc *func;
};

/* Standard intraprocedural DFA */

/* Reaching definition: assess domain size */
Array<Array<size_t>> var_to_assignment(var_pool_size);
size_t domain_size = 0;

for block in blocks {
    for each assignment instruction {
        var_to_assignment[instruction->lhs].push(instruction->ind);
        domain_size++;
    }
}

/* Reaching definition: Encode bitarray and create
 * data flow objects */
void _blabla(Block *block) {
    if (bl0) _blabla(bl0);
    if (bl1) _blabla(bl1);

    BitArray gen = zeros;
    BitArray kill = zeros;
    for each instruction {
        gen = gen + (1 << instruction->ind) - kill;
        kill = kill + all in var_to_assignment[instruction.var.ind] except instruction;
    }

    DataFlowObject *obj = new_dfa_obj();
    obj->block = block;
    obj->gen = gen;
    obj->kill = kill;
    obj->in = zeros;
    obj->out = zeros;
}

void dfa(
    DataFlowObject *entry_obj,
    Array<DataFlowObject *>objs,  // in reverse postorder
    BitArray *top,
    (BitArray *) MeetFunc *meet
) {
    entry_obj.out = top;
    bool changed = true;

    while (changed) {
        changed = false;

        for obj in objs {
            BitArray old_in = obj.in;
            BitArray old_out = obj.out;
            obj.in = meet(p.out for p in predecessors);
            if (old_in == obj.in) continue;

            obj.out = obj.func(obj);
            if (old_out != obj.out) changed = true;
        }
    }
}

/* Andersen DFA */
