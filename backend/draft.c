/* Instructions: doubly linked list
 * - type: quad, unary, assign, assign_acc, conditional jump, nonconditional jump, label
 * - based on type, different operand and operator objects
 * - prev
 * - next
 * - block
 * 
 * Basic Blocks: doubly linked list
 * - first and last instruction pointer
 * - successors array
 * - predecessors array
 * - ENTRY, EXIT tag
 * - visited tag?
 * 
 * Bit Array:
 * - union(BitArray), intersection(BitArray): vector | and &
 * - add_bitind(size_t): add a bitind to construct a BitArray
 * - has_bitind(size_t): bitind is 1
 * - subtr(BitArray): vector -, a & (~b)
 * - all - all 1's; none - all 0's
 * - equal - all bits match
 * 
 * Operand:
 * - Need to hold information about the number of bits
 * required to hold it, and whether it represents the name
 * of an address or the address itself
 * (either by linking to symbol,
 * or by copying the symbol info into itself)
 * 
 * Double Dict:
 * - Map where both sides are indexed. Key to value
 * is a one-to-many relationship.
 * - Querying by value returns all keys that map to the value
 * 
 * Assumptions:
 * - Label has pointers to instruction and block objects
 * 
 * TODO design:
 * - make instructions doubly-linked is clunky, how to eliminate
 * - copy instructions to block's instruction arrays, or hold pointers to the OG instruction array / pool?
 * - regular vs. floating point registers
 * - SSA:
 *  - Does not require DFA for use def calculations,
 *    just need hash map bookkeeping
 *  - Does not ever "kill"
 *  - No need to recalculate use def during any phase,
 *    incl. optimization & instruction selection.
 *    Just need to edit use def hash map on the fly
 *  - tradeoff: no need for EXIT but here we assume all
 *    programs should halt, and catch programs that
 *    absolutely do not halt
 */

#include "block.h"
#include "instruction.h"

#define BITARRAY_UNIT sizeof(__UINT64_TYPE__)

/* Optional: CFG simplification between Step 1 and 2 */

/* Step 2: DFA */

/* Input is blocks in rdst order; outputs are IN and OUT dicts */
Dicts live_variable_analysis(Array *blocks) {

    /* Encode all variables to bits. Assume each unique
     * var name represents a unique variable. */
    DoubleDict<char*, size_t> bitmap;
    size_t bitind = 0;
    for each block {
        for each instruction {
            for each var {
                if (var->name not in bitmap) {
                    bitmap[var->name] = bitind++;
                }
            }
        }
    }
    size_t bitarray_size = bitind / BITARRAY_UNIT + (bitind % BITARRAY_UNIT ? 1 : 0);

    Dict<size_t, BitArray> def;
    Dict<size_t, BitArray> use;
    Dict<size_t, BitArray> in;
    Dict<size_t, BitArray> out;

    /* Initialize use, def, in, out for each block */
    /* In the case of a[i] = b, a, i, b are used; nothing is defined. */
    /* In the case of x = x + y, x is used; x is defined. */
    for each block {
        use[block] = bitarray(bitarray_size).zeros();
        def[block] = bitarray(bitarray_size).zeros();
        in[block] = bitarray(bitarray_size).zeros();
        out[block] = bitarray(bitarray_size).zeros();

        /* Note that use process first, then define,
         * to take care of scenarios where a variable is
         * used and defined in the same instruction */
        for each instruction {
            /* If a var is used and not defined, add to use */
            for each used var {
                if (!def[block].has(bitmap[var->name])) {
                    use[block].add(bitmap[var->name]);
                }
            }
            /* If a var is defined, add to def */
            def[block].add(bitmap[def_var->name]);
        }
    }

    /* Iterative analysis for live variables */
    bool changed;
    do {
        changed = false;

        for (size_t i = blocks.size() - 2; i >= 0; i--) {
            /* OUT[b] = union of IN[s] for all successors */
            BitArray curr = bitarray(bitarray_size).zeros();
            for (size_t j = 0; j < blocks[i]->successors.size(); j++) {
                size_t s = blocks[i]->successors[j];
                out[blocks[i]] = curr.union(in[s]);
            }

            /* IN[b] = use U (OUT[b] - def) */
            new_in = use[blocks[i]].union(out[blocks[i]].subtr(def[blocks[i]]));

            /* Track if IN[b] has changed */
            if (!new_in.equal(in[blocks[i]])) {
                in[blocks[i]] = new_in;
                changed = true;
            }
        }
    } while (changed);

    /* TODO: Decode output? */

    return in, out;
}

/* Step 3: Local optimization */

/* TODO: implement DoubleDict to query by index in O(1).
 * For now, for clarity, use O(n) pass to remove items. */
void optimize_block(Block *block, Set<char*> *live) {
    /* Map expression to its last evaluation
     * before any redefinition of variables on either side.
     * Note that key-value pairs here have a 1-1 relationship.
     *
     * Examples:
     * x = y + z -> {(ADD, y, z) -> x}
     * x = a[i] -> {(ACCESS, a, i) -> x}
     * a[i] = x -> {(ACCESS, a, i) -> x} */
    DoubleDict<(char*, char*, char*), Operand*> last_eval;

    /* Map defined variable to its last copied variable
     * before any redefinition of either variable.
     * Key-value pairs have a 1-many relationship. E.g.,
     * both x -> b and y -> b can coexist. */
    DoubleDict<char*, Operand*> last_copy;

    /* Forward pass */
    for each instruction {
        switch (instruction->type) {
            case EXPR: /* x = y + z */

                /* Copy propagation */
                Operand *a = last_copy[instruction->operand1->name];
                if (a) instruction->operand1 = *a;

                Operand *b = last_copy[instruction->operand2->name];
                if (b) instruction->operand2 = *b;

                /* Arithmetic strength reduction:
                * Reduce algebraic identity: x * 1, x / 1, x + 0, x - 0, x ^ x, etc.
                * TODO: Reduce to bit arithmetic if x is int: x * 2 = x << 1, etc.
                * TODO: Evaluate constant expressions: on target machine */
                if (instruction->operator is mul or divide && either operand is constant and equals 1, and the other is not constant) {
                    instruction->type = COPY;
                    instruction->operator = COPY;
                    instruction->operand = the other operand;
                }
                else if (instruction->operator is add or sub && either operand is constant and equals 0, and the other is not constant) {
                    instruction->type = COPY;
                    instruction->operator = COPY;
                    instruction->operand = the other operand;
                }

                /* Common expression elimination:
                 * reduce the common expression to a copy operation */
                Operand *lhs = lvm[(operator->name, operand1->name, operand2->name)];
                if (lhs) {
                    instruction->type = COPY;
                    instruction->operator = COPY;
                    instruction->operand = *lhs; // a copy, not the original
                }

                /* Kill all last_eval items where the LHS variable
                 * appears anywhere in key or value
                 *
                 * Examples:
                 * x = a + b
                 * x = c + d
                 * y = a + b (cannot replace a + b with x)
                 * 
                 * y = x + a
                 * x = c + d
                 * z = x + a (cannot replace x + a with y)
                 * 
                 * a[i] = x
                 * x = c + d
                 * y = a[i] (cannot replace a[i] by x)
                 * 
                 * a[i] = x
                 * a = c + d
                 * y = a[i] (cannot replace a[i] by x)
                 * 
                 * a[i] = x
                 * i = c + d
                 * y = a[i] (cannot replace a[i] by x)
                 */
                char *lhs_name = instruction->lhs->name;
                for item in last_eval {
                    if (lhs_name == key.operand1.name || lhs_name == key.operand2.name || lhs_name == value->name) {
                        last_eval.remove(item);
                    }
                }

                /* Kill all last_copy items where the LHS variable
                 * appears in either key or value
                 *
                 * Examples:
                 * x = y
                 * x = a + b
                 * z = x + 1 (cannot replace x by y)
                 * 
                 * x = y
                 * y = a + b
                 * z = x + 1 (cannot replace x by y)
                 */
                for item in last_copy {
                    if (lhs_name == key || lhs_name == value->name) {
                        last_copy.remove(item);
                    }
                }

                /* Update last_eval */
                last_eval[(instruction->operator, instruction->operand1->name, instruction->operand2->name)] = instruction->lhs;

                break;

            case EXPR_ARR: /* x = a[i] */

                /* Copy propagation */
                Operand *a = last_copy[instruction->operand1->name];
                if (a) instruction->operand1 = *a;

                Operand *b = last_copy[instruction->operand2->name];
                if (b) instruction->operand2 = *b;

                /* Common expression elimination:
                 * reduce the common expression to a copy operation.
                 * The following cases apply:
                 * a[i] = b; x = a[i] -> x = b.
                 * x = a[i]; y = a[i] -> y = x. */
                Operand *lhs = lvm[(operator->name, operand1->name, operand2->name)];
                if (lhs) {
                    instruction->type = COPY;
                    instruction->operator = COPY;
                    instruction->operand = *lhs; // a copy, not the original
                }

                /* Kill all last_eval items where the LHS variable
                 * appears in either key or value
                 *
                 * Examples:
                 * x = a + b
                 * x = y[i]
                 * z = a + b (cannot replace a + b by x)
                 * 
                 * a = x + b
                 * x = y[i]
                 * z = x + b (cannot replace x + b by a)
                 * 
                 * a[i] = x
                 * x = c[j]
                 * y = a[i] (cannot replace a[i] by x)
                 * 
                 * a[i] = x
                 * a = c[j]
                 * y = a[i] (cannot replace a[i] by x)
                 * 
                 * a[i] = x
                 * i = c[j]
                 * y = a[i] (cannot replace a[i] by x)
                 */
                char *lhs_name = instruction->lhs->name;
                for item in last_eval {
                    if (lhs_name == key.operand1.name || lhs_name == key.operand2.name || lhs_name == value->name) {
                        last_eval.remove(item);
                    }
                }

                /* Kill all last_copy items where the LHS variable
                 * appears in either key or value
                 *
                 * Examples:
                 * x = y
                 * x = z[i]
                 * z = x + 1 (cannot replace x by y)
                 * 
                 * y = x
                 * x = z[i]
                 * a = y + 1 (cannot replace y by x)
                 */
                for item in last_copy {
                    if (lhs_name == key || lhs_name == value->name) {
                        last_copy.remove(item);
                    }
                }

                /* Update last_eval */
                last_eval[(ACCESS, instruction->operand1->name, instruction->operand2->name)] = instruction->lhs;

                break;

            case COPY: /* x = y */

                /* Copy propagation */
                Operand *a = last_copy[instruction->operand->name];
                if (a) instruction->operand = *a;

                /* Kill all last_eval items where the LHS variable
                 * appears in either key or value
                 *
                 * Examples:
                 * x = a + b
                 * x = y
                 * z = a + b (cannot replace a + b by x)
                 * 
                 * a = x + b
                 * x = y
                 * z = x + b (cannot replace x + b by a)
                 * 
                 * a[i] = x
                 * x = y
                 * z = a[i] (cannot replace a[i] by x)
                 * 
                 * a[i] = b
                 * a = c
                 * x = a[i] (cannot replace a[i] by b)
                 * 
                 * a[i] = b
                 * i = c
                 * x = a[i] (cannot replace a[i] by b)
                 */
                char *lhs_name = instruction->lhs->name;
                for item in last_eval {
                    if (lhs_name == key.operand1.name || lhs_name == key.operand2.name || lhs_name == value->name) {
                        last_eval.remove(item);
                    }
                }

                /* Kill all last_copy items where the LHS variable
                 * appears in either key or value
                 *
                 * Examples:
                 * x = y
                 * x = z
                 * z = x + 1 (cannot replace x by y)
                 * 
                 * y = x
                 * x = z
                 * a = y + 1 (cannot replace y by x)
                 */
                for item in last_copy {
                    if (lhs_name == key || lhs_name == value->name) {
                        last_copy.remove(item);
                    }
                }

                /* Update last_copy */
                last_copy[instruction->lhs->name] = instruction->operand;

                break;

            case COPY_ARR: /* a[i] = x */

                /* Copy propagation */
                Operand *a = last_copy[instruction->operand1->name];
                if (a) instruction->operand1 = *a;

                Operand *b = last_copy[instruction->operand2->name];
                if (b) instruction->operand2 = *b;

                Operand *c = last_copy[instruction->operand3->name];
                if (c) instruction->operand3 = *c;

                /* Kill all last_eval items where the array
                 * variable matches the array variable in an
                 * ACCESS expression
                 *
                 * Example:
                 * x = a[j]
                 * a[i] = b
                 * y = a[j] (cannot replace a[j] by x)
                 * 
                 * a[i] = x
                 * a[j] = c
                 * y = a[i] (cannot replace a[i] by x)
                 * 
                 * Note that, if the expression operator is
                 * not ACCESS, or if the array
                 * variable appears as a value in last_eval,
                 * the array variable won't kill the
                 * expression. E.g.,
                 * x = a + c
                 * a[i] = b
                 * y = a + c (can replace a + c by x)
                 * 
                 * a = x + y
                 * a[i] = b
                 * z = x + y (can replace x + y by a)
                 * 
                 * Similarly, the array variable cannot
                 * kill last_copy items involving itself:
                 * a = b
                 * a[i] = c
                 * d = a (can replace a with b)
                 * 
                 * b = a
                 * a[i] = c
                 * d = b (can replace b with a)
                 */
                char *array_name = instruction->array_var->name;
                for item in last_eval {
                    if (key.operator == ACCESS && array_name == key.operand1.name) {
                        last_eval.remove(item);
                    }
                }

                /* Update last_eval */
                last_eval[(ACCESS, array_name, instruction->index_var->name)] = instruction->operand;

                break;

            case COND: /* if x < y goto L */

                /* Copy propagation */
                Operand *a = last_copy[instruction->operand1->name];
                if (a) instruction->operand1 = *a;

                Operand *b = last_copy[instruction->operand2->name];
                if (b) instruction->operand2 = *b;

                /* Constant folding */
                if (instruction->operand1 is constant && instruction->operand2 is constant) {
                    /* TODO: fold */
                    /* Conditional -> unconditional */
                    /* Update branching */
                }

                break;

            default: // do nothing
        }
    }

    /* Backward pass for dead code removal */
    Instruction *instruction = block->last;

    while (instruction) {
        Instruction *prev = instruction->prev;
        Instruction *next = instruction->next;
        bool to_remove = false;

        switch (instruction->type) {
            case EXPR: /* x = y + z */

                /* Remove instruction if x is not live */
                if (!live[instruction->lhs->name]) {
                    to_remove = true;
                    break;
                }

                /* Kill x, keep y, z live */
                live.remove(instruction->lhs->name);
                live.add(instruction->operand1->name);
                live.add(instruction->operand2->name);

                break;

            case EXPR_ARR: /* x = y[i] */

                /* Remove instruction if x is not live */
                if (!live[instruction->lhs->name]) {
                    to_remove = true;
                    break;
                }

                /* Kill x, keep y, i live */
                live.remove(instruction->lhs->name);
                live.add(instruction->operand1->name);
                live.add(instruction->operand2->name);

                break;

            case COPY: /* x = y */

                /* Keep instruction if x is live */
                if (!live[instruction->lhs->name]) {
                    to_remove = true;
                    break;
                }

                /* Kill x, keep y live */
                live.remove(instruction->lhs->name);
                live.add(instruction->operand->name);

                break;

            case COPY_ARR: /* a[i] = b */

                /* Keep instruction if a is live */
                if (!live[instruction->array_var->name]) {
                    to_remove = true;
                    break;
                }

                /* Keep a, i, b live */
                live.add(instruction->array_var->name);
                live.add(instruction->ind_var->name);
                live.add(instruction->rhs->name);

                break;

            case COND: /* if x < y goto L */

                live.add(instruction->operand1->name);
                live.add(instruction->operand2->name);

                break;

            default:
        }

        if (to_remove) {
            if (next) next->prev = prev;
            if (prev) prev->next = next;
        }
        instruction = prev;
    }
}

void local_optimization(Array *blocks, Dict<size_t, Set<char*>> *out) {
    for each block in blocks {
        optimize_block(block, out[i]);
    }
}

/* Optional global optimizations.
 * E.g., constant folding, then, do a local sweep
 * to reduce any conditional jumps to unconditional jumps,
 * and eliminate relevant paths, if applicable.
 * Then, run CFG simplification. */

Array *optional_global_pass(Array *blocks) {
    /* Bla, bla... */

    entry, exit, block_size = simplify_blocks(blocks);
    return rdst(entry, exit, block_size);
}

/* Step 5: Identify webs
 * This is needed because there may be definitions
 * to the same variable without SSA. This step
 * separates out these cases into different variables. */

/* TODO: array has no first definition yet in our IR.
 * If a has no reaching definition in a[i] = b or
 * x = a[i], add a dummy definition so the algorithm
 * works, and these references of a share the same
 * register unless a is redefined through a = bla bla. */
void assign_virtual_registers(Array *blocks) {
    /* Assume that a preprocessing step finds all
     * definitions in a program, and builds a dict
     * that maps each variable name to a bitarray
     * of corresponding definitions */
    Dict<char*, BitArray> var;

    /* TODO: Global reaching definition analysis... */

    /* Output is, IN[b] for each block */
    Dict<size_t, BitArray> in;

    /* Also have gen and kill for each instruction already.
     * In case of a[i] = b, gen and kill are both empty. */
    Dict<Instruction*, BitArray> gen;
    Dict<Instruction*, BitArray> kill;

    /* Initialize index dict for union-find */
    Dict<Operand*, Operand*> *root;

    for each block other than entry and exit {
        for each instruction {
            /* Perform union (as in union find) on each used operand
             * and its respective reaching definitions */
            for each used operand {
                BitArray defs_bit = in[block].inter(var[operand->name]);
                Set<Instruction*> defs = defs_bit.decode();
                for (def in defs) union(def->lhs, used_var);
            }
            /* If the instruction is a definition,
             * use itself as a root for union find algorithm */
            if (instruction->lhs) {
                root[instruction->lhs] = instruction->lhs;
            }
            /* Update IN[i] for next instruction */
            in[block] = gen[instruction].union(in[block].substr(kill[instruction]));
        }
    }

    /* Update operand with its new identity */
    for each block other than entry and exit {
        for each instruction {
            for each operand {
                instruction->operand = find(operand);
            }
            /* TODO: remove trivial instructions */
            if (instruction is like "LD r1, r1") {
                remove instruction;
            }
        }
    }
}

/* Step 6: Instruction selection */

/* Input: blocks in reverse postorder */
void instruction_selection(Array *blocks) {
    /* For each block, for each instruction,
     * naively translate to machine instruction.
     * One IR instruction can map to multiple lines
     * of machine instructions. */

    /* Run use def globally */
    
    /* For each block, for each instruction,
     * check combination opportunity by looking at 
     * reaching definitions of operands. If there is
     * combination opportunity, and the operands
     * only have one reaching definition each, and
     * these reached definitions only have one use,
     * combine, update use defs of relevant blocks (?)
     * while worklist not empty, continue the process (?) */
}

/* Step 7: Instruction scheduling by block */
void instruction_scheduling(Array *blocks) {
    for (block : blocks) run_list_scheduling(block);
}

/* Step 8: Live variable analysis on virtual registers */
void live_variable_analysis_again(Array *blocks) {}

/* Step 9: Register allocation via graph coloring */

/* Step 10: Peephole pass 1 */

/* Step 11: Instruction scheduling again */

/* Step 12: CFG flattening and code modifications */

/* Step 13: Output program */
