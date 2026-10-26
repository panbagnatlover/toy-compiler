/* Instruction selection.
 * TODO: design decision to make vreg a pointer
 * in each instruction because each vreg has
 * a heap-allocated set attribute, and copying
 * heap set is impractical. Or, do we want BitSet?
 * TODO: hash set -> links. Revisit question above.
 * TODO: alias comparison on bitmap instead of hash set
 * TODO: at the end, run a round of CFG simplification. */

#include "block.h"
#include "mem.h"

#define MAX_PROCESS_COUNT 5

/* Translate a block of IR instructions
 * to machine instructions. */
void selection(Array<Block *> blocks, rules?) {
    Dict<Block *, MBlock *>map;

    for (block in blocks) {
        map[block] = newmblock();
    }

    /* Lower each IR instruction to machine
     * instruction and push to worklist. */
    PriorityQueue<Instruction *> worklist;

    for (block in blocks) {
        map[block]->bl0 = block->bl0 ? map[block->bl0] : NULL;
        map[block]->bl1 = block->bl1 ? map[block->bl1] : NULL;
        for (p in block->prececessors) {
            map[block]->predecessors.add(map[p]);
        }

        if (block->type == ENTRY || block->type == EXIT) {
            MBlock->type = block->type;
            continue;
        }

        Instruction *ins = block->head;
        MInst *m_next = NULL;
        MInst *m_prev = NULL;

        /* For each instruction in block,
         * lower it to machine instructions, set up
         * linkage, and add the machine instructions
         * to worklist. */
        while (ins) {
            m_next = lower(ins);
            m_next->prev = m_prev;
            if (m_prev) m_prev->next = m_next;
            while (m_next) {
                worklist.push(m_next);
                m_next->block = map[block];
                m_prev = m_next;
                m_next = m_next->next;
            }
            ins = ins->next;
        }
    }

    /* TODO: reaching definition analysis on vreg, then set up
     * use def and def use chain. Encode the domain of virtual registers
     * into a bitset, and tag def / use for each block.
     * Use def chain can be set up by looking at the reaching
     * definition at each instruction and populating defs for
     * the use, and use for the defs. Use hash set first,
     * then build chains. */

    /* TODO: build memory chain. */

    Set<MInst *> removed;
    Set<MInst *> added;
    Array<MInst *> changed;

    while (!worklist.empty()) {
        MInst *ins = worklist.top();
        worklist.pop();

        added.remove(ins);
        if (removed.contains(ins)) continue;

        /* Cap process count by MAX_PROCESS_COUNT to
         * avoid infinite recursion. */
        ins->process_count++;

        /* TODO: need to incrementally change block def and use? */
        for (rule in rules[ins->type]) {
            if (!checkEligibility(ins, rule)) continue;

            Instruction *new_ins = fold(ins, rule, changed, removed);
            for (c in changed) {
                if (!added.contains(c)) {
                    worklist.add(c);
                    added.add(c);
                }
            }
            changed.reset();
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
/* TODO: pass down alias info. */
/* TODO: live variable analysis after instruction
 * selection? How live interval is tackled */
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

bool checkEligibility(Instruction *ins, Rule rule) {

}

Instruction *fold(Instruction *instruction, Rule rule, Set<Instruction *>changed) {

}

bool dominate(MInst *a, MInst *b) {
    /* TODO: return dominate(a->block, b->block) */
}

/* TODO: refine. include or exclude boundaries */
bool no_redef_or_use_between(VR r, MInst *a, MInst *b) {
    /* Block of instruction a must dominate
     * block of instruction b */
    if (!dominate(a->block, b->block)) return false;

    /* If a, b share the same block, check r locally */
    if (a->block == b->block) {
        /* If r is not in def or use bitset, return true */
        if (!a->block->def[r] && !a->block->use[r]) return true;

        /* Otherwise, check instruction by instruction */
        MInst *n = a->next;
        while (n && n != b) {
            if (r used in n or r defined in n) return false;
        }
        if (n == b && no use or redef at b) return true;
        return false;
    }
    
    /* If a, b share different blocks, traverse all
     * intermediate blocks from b to a and check
     * use / def bitsets. */
    Queue<Block *> blocks;
    Set<Block *> added;
    blocks.add(b->block);
    added.add(b->block);

    while (!blocks.empty()) {
        Block *f = blocks.front();
        blocks.pop();

        /* If r is redefined or used in the use / def bitset,
         * check if r is in a->block or b->block,
         * and determine if r is between a, b */
        if (f->def[r] || f->use[r]) {
            if (f == b->block) {
                /* TODO: check if r is defined or used before b */
                if so, return false;
            }
            else if (f == a->block) {
                /* TODO: check if r is defined or used after a */
                if so, return false;
            }
            else return false;
        }

        /* Continue adding intermediate blocks to the
         * queue until only a->block remains. Since a->block
         * dominates b->block, a->block must be
         * the last remaining block in the end. */
        if (f != a->block) {
            for (p in a->block->predecessors) {
                if (!added.contain(p)) {
                    blocks.push(p);
                    added.add(p);
                }
            }
        }
    }

    return true;
}

bool rule1_eligible(MInst *e) {
    /* Validate r has only one definition. */
    if (e->r1->defs.size() != 1) return false;
    VReg *r = e->r1->defs[0];
    MInst *s = r->inst;

    /* Validate s is the form of ADD r, x, y. */
    s is of the form ADD r, x, y;

    /* Validate r has no other use. */
    if (r->uses.size() != 1) return false;

    /* Validate s dominates entry. */
    if (!dominate(s, entry)) return false;

    if (x is VR && !no_redef_between(x, s, e)) return false;
    if (y is VR && !no_redef_between(y, s, e)) return false;

    return true;
}

void rule1_combine(MInst *entr, Set<MInst *> &changed, Set<MInst *> &removed) {
    MInst *s = entry->r1->def[0];

    replace entry instruction with LDR;
    entry->r3 = s->r3;
    entry->r4 = s->r4;
    entry->r3->inst = entry;
    entry->r4->inst = entry;

    /* Remove s. Since r1 has no other use,
     * s is safe to delete. No def chain needs
     * to be updated. */
    MInst *s_next = s->next;
    MInst *s_prev = s->prev;
    if (s_next) s_next->prev = s_prev;
    if (s_prev) s_prev->next = s_next;
    s->block = NULL;

    removed.add(s);

    /* Changed: entry, r3 defs, r4 defs, entry uses.
     * TODO: add back to worklist. */
    if (entry->process_count < MAX_PROCESS_COUNT)
        changed.add(entry);
    for (def in entry->r3->defs) {
        if (def->inst->process_count < MAX_PROCESS_COUNT)
            changed.add(def->inst);
    }
    /* TODO: same for r4 */
    for (use in entry->r2->uses) {
        if (use->inst->process_count < MAX_PROCESS_COUNT)
            changed.add(use->inst);
    }
}

/* Returns MUST_ALIAS, MAY_ALIAS, or NO_ALIAS. */
AliasType alias(VMem *a, VMem *b) {
    /* If both are variables, directly compare the variables. */
    if (a->type == b->type && a->type == VAR) {
        return a->var == b->var ? MUST_ALIAS : NO_ALIAS;
    }

    /* If one is a variable, check if it belongs to the
     * alias set of the other */
    if (a->type == VAR && b->type != VAR) {
        if b->must_alias.contains(a->var) return MUST_ALIAS;
        if b->may_alias.contains(a->var) return MAY_ALIAS;
        return NO_ALIAS;
    }
    if (a->type != VAR && b->type == VAR) {
        if a->must_alias.contains(b->var) return MUST_ALIAS;
        if a->may_alias.contains(b->var) return MAY_ALIAS;
        return NO_ALIAS;
    }

    /* If both are VReg offsets, check if they are the same. */
    if (a->base == b->base) {
        /* TODO - check constant offset, reg offset. */
        return MUST_ALIAS;
    }

    /* Check if a's and b's alias sets overlap directly. */
    if a->must_alias.intersects(b->must_alias) return MUST_ALIAS;
    if a->may_alias.intersects(b->may_alias) return MAY_ALIAS;

    /* Check if a may totally or partially overlap with b.
     * TODO: optimize quadratic algorithm. */
    for (l in a->may_alias) {
        for (r in b->may_alias) {
            if (
                l->type == PT_OFFSET
                && r->type == PT_OFFSET
                && l->base == r->base
                && l->offset + l->size > r->offset
                && l->offset < r->offset + r->size
            ) return MAY_ALIAS;
        }
    }

    return NO_ALIAS;
}

/* NOTE:
 * When lowered to MInst, STR memory is either
 * a variable (with its own size, offset) or
 * a [reg, reg] or [reg, const]. In the case
 * of [reg, reg/const], need to assess:
 *  - if compared to another [reg], pt() overlap
 *  - if compared to var, whether var is a base in
 *    in pt(reg) and size overlap maybe
 * In the case of var, compare to another var,
 * just need to ensure the two vars are the same.
 */
/* TODO: we also don't have rules that manipulate
 * memory offsets because alias analysis is
 * set in stone before backend. */
bool rule4_eligible(MInst *entry) {
    /* Validate r2 only has one use. */
    if (entry->r2->uses.size() != 1) return false;
    VReg *n_r2 = entry->r2->uses[0];

    /* Validate r2 has only one definition. */
    if (n_r2->defs.size() != 1) return false;

    /* Validate entry dominates r2 use. */
    if (!dominate(entry, n_r2->inst)) return false;

    /* Walk up the memory chain and check
     * the alias type of instructions. */
    Queue<MInst *> worklist;
    Set<MInst *> added;
    for (p in entry->predecessors) {
        worklist.add(p);
        added.add(p);
    }

    MInst *dominator = NULL;
    int steps = 0;
    int MAX_STEPS = 10;

    while (!worklist.empty()) {
        /* If cannot find an eligible dominator
         * in MAX_STEPS, abort. */
        steps++;
        if (steps == MAX_STEPS) return false;

        MInst *top = worklist.front();
        worklist.pop();

        if (top is a STR instruction) {
            AliasType t = alias(entry->loc, top->loc);

            /* Abort when any may alias is encountered. */
            if (t == MAY_ALIAS) return false;

            /* Abort when a must alias is encountered,
             * but the STR instruction does not dominate
             * the LDR instruction. */
            if (t == MUST_ALIAS && !dominate(top, entry)) return false;

            /* If a dominating must alias is reached,
             * continue processing the worklist without
             * walking up the chain. Must evaluate other
             * paths to the dominating instruction does
             * not have aliases. */
            if (t == MUST_ALIAS) {
                dominator = top;
                continue;
            }
        }

        if (top is a STP instruction) {
            /* TODO */
        }

        /* Continue walking up the chain */
        if (top != dominator) {
            for (p in top->predecessors) {
                if (!added.contains(p)) worklist.add(p);
            }
        }
    }

    if (!no_redef_between(dominator->r1, dominator, n->block)) return false;
    if (!no_redef_between(entry->r2, entry, n->block)) return false;

    return true;  // TODO - return the dominator
}

void rule4_combine(MInst *entry, MInst *str, MInst *use, Set<MInst *> &changed, Set<MInst *> &removed) {
    replace r2 in use with str->r1;

    /* Remove the LDR instruction. */
    entry->inst = NULL;
    MInst *prev = entry->prev;
    MInst *next = entry->next;
    if (prev) prev->next = next;
    if (next) next->prev = prev;

    removed.add(entry);

    /* Update the memory chain. */
    connect predecessors to successors and vice versa;

    /* Register changed instructions. */
    changed.add(use);
    for (def in use->r1->defs) {
        if (def->inst->process_count < MAX_PROCESS_COUNT)
            changed.add(def->inst);
    }
}

/* TODO - what about flag safety? */
/* Rules

(1) For both LDR and STR
ADD r1, r3, r4
LDR r2, [r1] <- entry
->
LDR r2, [r3, r4]

If ADD dominates LDR,
r3, r4 have no redefinition between ADD and LDR,
r1 has no redefinition between ADD and LDR,
r1 has no other use,
then we can combine and place at LDR.
Delete ADD.

Prefer this rule:
If ADD dominates LDR,
r2 has no use or definition between ADD and LDR,
r1 has no redefinition between ADD and LDR,
r1 has no other use,
then combine and place at ADD.
Delete LDR.

Note that r1 cannot be redefined through a pointer,
because r1 is a virtual register. However,
r1 can be redefined in non-SSA IR through conditional flow,
e.g.,
y = 1
if (cond) y = 2
x = y

(2)
MUL r1, r2, r3
ADD r4, r1, r5 or ADD r4, r5, r1 <- entry
->
MADD r4, r2, r3, r5

If MUL dominates ADD,
r1 has no redefinition between MUL and ADD,
r2, r3 are not redefined between MUL and ADD,
combine and place at location of ADD,
If r1 has no use, delete MUL.

If MUL dominates ADD,
r4 has no use or redefinition between MUL and ADD,
r1 has no redefinition between MUl and ADD,
r1 has no other use,
then combine and place at MUL,
delete ADD.

(3)
MUL r1, r2, r3
SUB r4, r5, r1 <- entry
->
MSUB r4, r2, r3, r5

Same as above.

(4) Store-load forwarding
STR r1, loc
LDR r2, loc <- entry
instruction that uses r2
->
STR r1, loc
instruction that uses r1

If STR dominates LDR, LDR dominates
an instruction that uses r2, r1 and r2
share the same location and same size,
there's no other write to the same
location in between STR and LDR (e.g., 
if STR loc = mem(x, 4), ensure STP doesn't
write to a location that may overlap with loc
like mem(x, 0);
ensure any location written by a STR does
not alias with loc, analyzed through alias analysis),
no redefinition of r1 between STR and the instruction,
no redefinition of r2 between LDR and the instruction,
then replace r2 with r1 in the instruction.
Eventually, if r2 has no other use, delete the instruction.

Note that, e.g., let a, c be pointers.
a = b - i
c = a + j
pt(mem(a)) = mem(v, -i) for v in mem(b)
pt(mem(c)) = mem(v, -i + j)
STR r1, c may overwrite what's in a.

Example of load-store forwarding:

If we have a = b + 32; x = *a, but a is a stack variable,
then, we translate to 
1. ADD  rr0, x_b, #32      ; Compute (b + 32)
2. STR  rr0, [x29, #FI_a]  ; Store result into stack slot 'a'
3. LDR  rr1, [x29, #FI_a]  ; Read 'a' back out from stack slot
4. LDR  w_v, [rr1]         ; Dereference pointer
Apply store-load forwarding to remove 3, then apply earlier rule.

(5) For both LDR and STR
ADD  x1, x1, x2
LDR  x0, [x1] <- entry
->
LDR  x0, [x1, x2]!

If ADD dominates LDR,
no use or redefinition of x1 between ADD and LDR,
no redefinition of x2 between ADD and LDR,
combine instructions and place at LDR,
delete ADD.

If ADD dominates LDR,
no use or redefinition of x0 between ADD and LDR,
no redefinition of x1 between ADD and LDR,
combine instructions and place at ADD,
delete LDR.

Note that this scenario should not happen
right after macro expansion because no x = x + a
can exist after web building, but it could appear
as a result of earlier folds or register allocations.

(6) For both LDR and STR
LDR  x0, [x1]
ADD  x1, x1, x2 <- entry
->
LDR  x0, [x1], x2

If LDR dominates ADD,
no redefinition of x1 between LDR and ADD,
no use of x0 between LDR and ADD,
combine instructions and place at ADD,
delete LDR.

If LDR dominates ADD,
no redefinition of x2 between LDR and ADD,
no redefinition or use of x1 between LDR and ADD,
combine instructions and place at LDR,
delete ADD.

Note that this scenario should not happen
right after macro expansion because no x = x + a
can exist after web building, but it could appear
as a result of earlier folds or register allocations.

(7) Sign extension (what about floating point)
...

/* TODO: use def needs to be constantly updated. */
/* TODO: legalizer - e.g., offset number. */
/* TODO: linkage with prev use (5), (6) */

/* TODO: add a VirtualRegister struct type that functions
 * similarly to the MachineOperand type in LLVM. Separating
 * out VR vs stack locations is important in lowered code
 * because that's how machine instructions work. In IR,
 * everything is a generic operand. */

/* TODO: add to notes. Why put stack vs register lowering in IR?
 * Different architectures deal with it differently?
 * RISC loads to registers and CISC loads from stack directly

Every stack location needs to record the instructions
that may write to it. This info needs to be passed
down from IR somehow.
p = &x
q = p + 1
*(p + 1) = 1
*(q + 2) = 2

Dominator tree that allows O(1) querying. */

/* TODO - Handling Mid-Pass VReg Allocation (If New VRegs Are Created)
 * During combining, you might occasionally
 * create a new virtual register
 * (for example, splitting a 64-bit operation
 * into two 32-bit operations, or creating a
 * temporary constant register).
 * You do not need to re-run the whole tagging
 * pass when this happens:
 * Bitset Expansion: Simply resize the BitVector
 * when allocating a new VReg index
 * ($N \rightarrow N+1$).
 * Local Update: Set the bit for the new VReg
 * index in Def[CurrentBB].
 * Conservatism: Because the new VReg was
 * just created in CurrentBB, it didn't exist
 * in any prior basic block. Its bit remains 0
 * (false) across all other basic blocks,
 * which is mathematically exact!
 * Can overallocate initially to create some buffer
 * to avoid having to resize for all blocks. */
