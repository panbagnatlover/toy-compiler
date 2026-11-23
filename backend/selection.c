/* Instruction selection.
 * Each vreg in instruction is a flat struct, not pointer
 * because vreg is flow sensitive.
 * TODO: instruction on an arena or array? Arena is difficult
 * for conversion from bitarray to inst.
 * 
 * Treat x = &a as  LD r, a. a represents a stack location
 * and it is not known yet. Eventually will be ADD r, SP, #some_offset.
 * Treat x = *a as LD r1, [a]; LD r2, [r1].
 * [a], [r] both represent VMems.
 * [a] has pt set {a}, and [r] has pt set pt(a).
 * Treat x = a as LD r, [a].
 * 
 * TODO: need proof for alias func based on how analysis is done.
 * Proof1: if VMem overlaps, their pt_set overlaps.
 * Proof2: if 2 mems have diff base, they do not alias. */

#include "array.h"
#include "arena.h"
#include "block.h"
#include "memory.h"
#include "vmem.h"
#include "inst.h"
#include "dll.h"
#include "bitarray.h"
#include "reachingdef.h"
#include "rule.h"
#include "dominator.h"
#include "aliasmap.h"

#define MAX_PROCESS_COUNT 5

typedef struct {
    bool added;
    bool removed;
    uint8_t process_count;
} InstInfo;

/* Returns a new array. Old blocks and instructions
 * will be freed. */
Array *selection(Array *blocks, Context *c) {
    /* Mapping table from old block ind to new block ind */
    size_t *map = malloc(sizeof(size_t) * blocks->size);
    /* Array of new blocks in reverse postorder */
    Array *new_blocks = array(sizeof(Block), i);  // TODO: allocate size of blocks->size

    Block **arr = (Block **) new_blocks->arr;
    Block **old_arr = (Block **) blocks->arr;

    for (int i = 0; i < blocks->size; i++) {
        arr[i] = newblock(); // TODO: Should create in c->block_arena
        arr[i]->ind = i;
        map[old_arr[i]->ind] = i;
    }

    // TODO: add APIs to use array as a stack / queue
    // TODO: size should be at least INST_POOL_SIZE
    Array *worklist = array(sizeof(Inst *), i);

    for (int i = 0; i < blocks->size; i++) {
        /* Create linkage between blocks. */
        Block *block = old_arr[i];
        arr[i]->bl0 = block->bl0 ? arr[map[block->bl0->ind]] : NULL;
        arr[i]->bl1 = block->bl1 ? arr[map[block->bl1->ind]] : NULL;
        for (p in block->prececessors) {
            arr[map[block->ind]]->predecessors.add(arr[map[p->ind]]);
        }

        if (block->type == ENTRY || block->type == EXIT) {
            arr[i]->type = block->type;
            continue;
        }

        DLL *old_insts = &(block->insts);
        arr[i]->insts = (DLL){ NULL, NULL, 0 };
        DLL *insts = &(arr[i]->insts);

        /* For each instruction in block,
         * lower it to machine instructions, set up
         * linkage, and add the machine instructions
         * to worklist. */
        Inst *old_inst = (Inst *) old_insts->head;
        while (old_inst) {
            DLL *new_insts = lower(old_inst);
            push_dll(new_insts, insts);
            old_inst = old_inst->next;
        }
        Inst *inst = (Inst *) insts->head;
        while (inst) {
            inst->block = arr[i];
            push_array(&inst, worklist);
            inst = inst->next;
        }
    }

    /* TODO: free old blocks and old insts and map. */

    /* Analyze reaching definitions on vregs. */
    ReachingDefContext *rd = reaching_def(new_blocks);
    Array *dfa_objs_arr = rd->objs;
    Array *dfa_defs_arr = rd->defs;
    ReachingDefDFAObject *dfa_objs = (ReachingDefDFAObject *) dfa_objs_arr->arr;
    BitArray **dfa_defs = (BitArray **) dfa_defs_arr->arr;

    /* TODO: Encode the domain of virtual registers
     * into a bitset, and tag def / use for each block. */
    for (int i = 0; i < dfa_objs_arr->size; i++) {
        Block *block = dfa_objs[i].block;
        BitArray *defs = dfa_objs[i].in;
        Inst *inst = (Inst *) block->insts->head;
        while (inst) {
            BitArray *tmp = bitarray();
            /* For each used vreg, find the set of reached definitions
             * and set up use-def chains. */
            for (each used vreg) {
                size_t iter = 0;
                intersect_bitarray(dfa_defs[vreg->var->ind], defs, tmp);
                while(iter = iter_bitarray(tmp, iter)) {
                    Inst *def_inst = get inst with ind iter;
                    create_use_def_chain(def_inst->lhs, &vreg);
                    iter++;
                }
            }
        }
    }

    /* TODO: build memory chain. */

    /* Create inst info array to store temp metadata about
     * each instruction during the peephole process. */
    Array *inst_info_arr = array(sizeof(InstInfo), i); // TODO: inst pool size
    InstInfo *inst_info = (InstInfo *) inst_info_arr->arr;
    for (int i = 0; i < inst_info_arr->size; i++) {
        inst_info[i].process_count = 0;
        inst_info[i].added = false;  // Marks if an instruction is currently in queue
        inst_info[i].removed = false;  // Marks if an instruction is removed from the program
    }

    Array *changed = array(sizeof(Inst *), i);

    while (worklist->size) {
        Inst *inst = back_array(worklist);
        pop_array(worklist);

        inst_info[inst->ind].added = false;
        if (inst_info[inst->ind].removed) continue;

        /* Cap process count by MAX_PROCESS_COUNT to
         * avoid infinite recursion. */
        inst_info[inst->ind].process_count++;

        /* TODO: need to incrementally change block def and use? */
        Array *rules = get_all_rules(inst);
        for (rule_ind in rules) {
            Rule *rule = get_rule(rule_ind);
            if (!rule->check_eligibility(inst)) continue;
            rule->combine(inst, changed);
            for (inst_ind in changed) {
                if (!inst_info[inst_ind].added) {
                    add inst_ind to worklist;
                    inst_info[inst_ind].added = true;
                }
            }
            reset_array(changed);
            break;
        }
    }

    // /* TODO after function implementation
    //  * - Add prologue and epilogue */
    // MBlock *m_entry = map[b_entry];
    // MBlock *m_exit = map[b_exit];

    // /* TODO - create new blocks */
    // next ins = SUB SP, SP, #8;
    // next ins = STR FP, [SP];
    // next ins = MOV FP, SP;
    // next ins = SUB SP, SP, size;  // size TBD

	// // TODO - after register alloc, save callee-managed register state

    // // TODO - tear down, restore callee-managed register state
    // m_exit->head = MOV SP, FP;
    // next ins = LDR FP, [SP];
    // next ins = ADD SP, SP, #8;
    // next ins = br [SP];
    
    // create next block;
    // next ins = ADD SP, SP #8;
}

/* Macro expansion. */
/* TODO: pass down alias info. See brain dump after this. */
MInst *lower(Instruction *instruction) {
    switch (instruction->type) {

        case COPY:
            VReg r = newvreg();
            MInst *head;
            MINst *tail;
            if (lhs is register) copy lhs attributes to r;

            if (rhs is constant integer) {
                Int i = newint(rhs.value);
                r.type = REG_X;
                head = MOV r, i;
            }
            else if (rhs is constant fp) {
                Fp c = newfp(rhs.value);  // Creates a fp in constant pool, which will be keyed by a labeled location
                r.type = REG_V;
                head = LDR r, c;  // LDR r, =label
            }
            else if (rhs is register) {
                VReg r2 = newvreg();
                copy rhs attributes to r2;
                r.type = rhs.type;
                head = MOV r, r2;
            }
            else {
                Loc l = newloc();
                copy rhs attributes to l;
                r.type = l.type;
                head = LDR r, l;  // LDR r, [SP, offset]
            }

            if (lhs is register) tail = head;
            else {
                Loc l = newloc();
                copy lhs attributes to l;
                tail = STR r, l;  // STR r, [SP, offset]
                head->next = tail;
                tail->prev = head;
            }

            return head;

        case ...:
            ...

        case a[i] = b:
            if a, i, b are vreg:
                STR b, [a, i];
                // Set a[i] alias
            else if a is stack, i, b are vreg:
                LDR r, a;
                STR b, [r, i];
                // Set r[i] alias
            else if ...

        case a = b[i]:
            if a, b, i are vreg:
                LDR a, [b, i];
            else if a is stack, b, i are vreg:
                LDR r, [b, i];
                STR r, a;
            else if ...

        case COND:
            ...
            return;

        default:
            /* TODO - raise error */
    }
}

y = &a;
x1 = y + 1;
z1 = *x1;
x2 = y + 2;
z2 = *x2;


y = &a; // pt(y) = {a}
t = y + 1; // pt(t) = {a, 1};
*t = x; // val(a, 1) = val(x);
t2 = y + 2; // pt(t2) = {a, 2};
*t2 = z; // val(a, 2) = val(z);
k = &t; // pt(k) = {t};
t3 = y + 1; // pt(t3) = {a, 1};

x = 0x12345; // pt(x) = {0x12345}
*x = 1; // val(0x12345) = 1
ST [x], #0x12345; // Store to x.location, x represents (r1, r2), pt(mem) = {x}
LD r, [x];  // Load value at x.location
ST [r], #1;  // Store 1 to 0x12345

t = y + 1;
*t = x;
t2 = y + 2;
*t2 = z;
// If t, t2 are vregs
ADD t, y, 1;
ST [t], x; // Store to t.value; pt(mem) = pt(t) = {a, 1}
ADD t2, y, 2;
ST [t2], z; // Store to t2.value; pt(mem) = pt(t2) = {a, 2}
// If t, t2 are stack locs
ADD r, y, 1;
ST [<t>], r; // Store to t.loc; pt(mem) = t; mem.value = t.location
LD r2, [<t>];
ST [r2], x;  // Store to r2.value; pt(mem) = pt(t) = {a, 1}
ADD r3, y, 2;
ST [<t2>], r3; // Store to t2.loc; pt(mem) = t2
LD r4, [<t2>];
ST [r4], z;  // Store to r4.value; pt(mem) = pt(t2) = {a, 2}

ADD r1, y, 1;
ST r1, [mem1]; // pt(mem1) MUST= {(x, 0)}
LD r2, [mem1];
LD r3, [r2];  // pt(r2) = pt(x, 0)
