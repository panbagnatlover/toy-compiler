/* TODO - 
 * chunked bitarray vs normal (dense) bitarray.
 * Allow both to dynamically expand. */

/* Separate object because data flow objects
 * are temporary and do not get reused. */
#include <stdlib.h>

/* Capping the domain size to ~16M objects.
 * Each chunk consists of 256 bits, and a
 * chunked bit array can have 2^16 chunks. */
typedef struct {
    uint16_t ind;
    uint64_t bits[4];  // 256 bits
} Chunk;

typedef struct {
    Array<Chunk> chunks;
} ChunkedBitArray;

typedef struct {
    BitArray in;
    BitArray out;
    BitArray use;
    BitArray def;
    Block *block;
    TFunc *func;
} DataFlowObject;

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

/* Andersen DFA - domain is not known beforehand */
Allocate a fixed domain chunk to start;
global point-to map;

bool changed = true;
while global point-to map changed {
    changed = false;

    for obj in objs {
        update val set;
        if (new value) {
            create newInt(value); // NAI, Universe included
            update bit chunk;
        }

        update pt set;
        if (new memory) {
            create newMem(value);
            update bit chunk;
        }

        update change;
    }
}

/* Needed when bitarray needs to resize, e.g., during
 * MOV insertion in register allocation, live_in / live_out
 * sets may change in size. */
void update_bit_array(BitArray *array) {

}

void update_bit_chunk(ChunkedBitArray *array) {
    if has buffer, allocate to latest chunk of array;
    if no buffer {
        create a new chunk to array;
        allocate bit to the new chunk;
    }
}
