/* DEPRECATED */
#include <stdlib.h>
#include <stdbool.h>
#include "block.h"

/* TODO: input instruction array should be a fixed pool.
 * Instruction cjump and jump have NULL next. Their
 * successors have NULL prev. */

/* TODO: worth making worklist a set queue? */

/* TODO p2: set doubly links as indexed access as opposed
 * to pointer access. */

/* TODO: no instruction level linkage between blocks.
 * next, prev of terminating instructions 
 * are basically useless after blocks are set up. Block
 * successors need to specify branch. */

void buildCFG(ArrayPool *instructions) {

    /* Build sentinel instructions and blocks. */
    Instruction *i_entry = instruction(DUMMY_ENTRY);
    Instruction *i_exit = instruction(DUMMY_EXIT);
    Block *b_entry = block();
    Block *b_exit = block();

    b_entry->head = i_entry;
    b_entry->tail = i_entry;
    b_exit->head = i_exit;
    b_exit->tail = i_exit;
    i_entry->block = b_entry;
    i_exit->block = b_exit;

    /* Map each label to its target instruction. */
    Dict<Label *, Instruction *>map;
    Instruction *target = i_exit;
    for (int i = instructions.size() - 1; i >= 0; i--) {
        if (instructions[i].type == LABEL) {
            map[instructions[i].label] = target;
        }
        else target = instructions[i];
    }

    /* Set linkage between instructions and mark leaders.
     * A leader is an instruction that either follows
     * a jump instruction or is the target of a jump
     * instruction. */
    Instruction *prev = b_entry;
    Instruction *curr = b_entry;
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i].type == LABEL) continue;
        curr = instructions[i];

        switch (prev->type) {
            case JUMP:
                curr->is_leader = true;
                break;
            case CJUMP:
                curr->is_leader = true;
                curr->prev = prev;
                prev->next = curr;
                break;
            case FCALL:
                curr->is_leader = true;
                curr->prev = prev;
                prev->next = curr;
                break;
            default:
                curr->prev = prev;
                prev->next = curr;
        }

        Instruction *des;
        switch (curr->type) {
            case JUMP:
                des = map[curr->instruction.jump.label];
                curr->instruction.jump.jump_des = des;
                des->is_leader = true;
                break;
            case CJUMP:
                des = map[curr->instruction.cjump.label];
                curr->instruction.cjump.jump_des = des;
                des->is_leader = true;
                break;
        }
    }

    /* If the last instruction is not an unconditional
     * jump, link it to the exit instruction */
    if (curr->type != JUMP) {
        curr->next = i_exit;
        i_exit->prev = curr;
    }

    /* Early exit if no instructions exist */
    Instruction *first = i_entry->next;
    if (first == i_exit) return;

    /* Create a block for the first instruction
     * and link it to the entry block */
    Block *block = newblock();
    first->block = block;
    block->predecessors.add(b_entry);
    b_entry->successors.add(block);

    /* Create a block at each leader, update
     * the block field of each instruction, and set up
     * block linkage */
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i]->type == LABEL) continue;

        /* Set the leader instruction as the head
         * of a block */
        if (instructions[i]->is_leader) {
            if (!instructions[i]->block) {
                instructions[i]->block = newblock();
            }
            block = instructions[i]->block;
            block->head = instructions[i];
        }
        else instructions[i]->block = block;

        /* Handle jump instructions. All jump
         * instructions are terminating instructions */
        if (instructions[i]->type == JUMP) {
            block->tail = instructions[i];

            /* Set up a destination block if doesn't
             * have one yet */
            Instruction *des = instructions[i]->instruction.jump.jump_des;
            if (!des->block) des->block = newblock();

            /* Set up linkage with the destination block */
            block->successors.add(des->block);
            des->block.predecessors.add(block);
        }
        else if (instructions[i]->type == CJUMP) {
            block->tail = instructions[i];

            /* Set up a jumped-to block if doesn't
             * have one yet */
            Instruction *des = instructions[i]->instruction.cjump.jump_des;
            if (!des->block) des->block = newblock();

            /* Set up linkage with the jumped-to block */
            block->successors.add(des->block);
            des->block.predecessors.add(block);

            /* Set up the next block if doesn't
             * have one yet */
            Instruction *next = instructions[i]->next;
            if (!next->block) next->block = newblock();

            /* Set up linkage with the next block */
            block->successors.add(next->block);
            next->block.predecessors.add(block);
        }
        /* If the instruction is a terminating
         * instruction but not a jump instruction,
         * set it as the tail of the block */
        else if (
            instructions[i]->next->is_leader
            || instructions[i]->next == i_exit
        ) {
            curr->tail = instructions[i];

            /* Set up the next block if doesn't
             * have one yet */
            Instruction *next = instructions[i]->next;
            if (!next->block) next->block = newblock();

            /* Set up linkage with the next block */
            block->successors.add(next->block);
            next->block.predecessors.add(block);
        }
    }
}

/* Helper function to append blocks in postorder DFS order */
void _postorder(Block *block, Set<Block*> *visited, Array *array) {
    visited.add(block);

    for (successor in block->successors) {
        if (!visited.contains(successor)) {
            _postorder(successor, visited, array);
        }
    }

    array.add(block);
}

/* Return array of blocks in reverse postorder. */
Array *orderCFG(Block *entry) {
    Array *block_array;
    Set<Block*> visited;

    _postorder(entry, &visited, block_array);

    if (exit is not in array) {
        /* TODO: program warning: program does not halt */
    }
    return reverse(block_array);
}

/* CFG simplification: analyze the reachable blocks
 * and repeatedly eliminate redundant jumps
 * and collapse redundant blocks. At every iteration,
 * add changed blocks back to the worklist. Repeat
 * until worklist is empty. */
Array<Block *> simplifyCFG(Block *b_entry) {
    PriorityQueue<Block *> worklist;

    /* Add reachable blocks other than entry and exit
     * to worklist. */
    Array<Block *> *reached = orderCFG(b_entry);
    for (block in reached) {
        if (block != b_entry && block->type != EXIT)
            worklist.push(block);
    }

    while (!worklist.empty()) {
        Block *block = worklist.top();
        worklist.pop();

        if (block == b_entry || block == b_exit) continue;
        if (!block->predecessors) continue;

        /* If a block is empty, remove the block from
         * CFG and connect its predecessors to its
         * successor. */
        if (!block->head) {

            /* An empty block can at most have 1 successor.
             * Otherwise, a structural error has occurred
             * earlier in the program. */
            if (block->successor.size() > 1) {
                /* TODO: throw error */
            }

            Instruction *s = block->successor[0];

            /* An empty block cannot be in a
             * self-recursive loop. Otherwise, a
             * structural error has occurred earlier
             * in the program. */
            if (block in block->predecessor || block == s) {
                /* TODO: throw error */
            }

            /* Set up linkage between blocks. No
             * instruction-level linkage needs to be
             * modified. */
            s->predecessors.remove(block);
            for (p in block->predecessors) {
                p->successors.remove(block);
                p->successors.add(s);
                s->predecessors.add(p);

                /* Add predecessor to worklist.
                 * Blocks with modified linkages may have
                 * further optimization opportunities. */
                worklist.push(p);
            }

            /* Make the block unreachable. */
            block->predecessors.reset();

            /* Add successor to worklist.
             * Blocks with modified linkages may have
             * further optimization opportunities. */
            worklist.push(s);
        }

        /* Assess opportunity to eliminate redundant jumps
         * when the tail of a block is a conditional jump. */
        else if (block->tail->type == CJUMP) {
            Instruction *jump_des = block->tail->instruction.cjump.jump_des;
            Instruction *next = block->tail->next;

            /* Skip if the conditional jump is in a
             * self-recursive loop. */
            if (jump_des == block->tail || next == block->tail) continue;

            /* If the jumped-to instruction and the next
             * instruction are the same, remove the 
             * conditional jump and update linkage. 
             * Note that it is unnecessary to convert
             * the conditional jump to an unconditional
             * one. */
            if (jump_des == next) {
                Instruction *del = block->tail;
                del->block = NULL;

                if (del->prev) {
                    del->prev->next = next;
                    next->prev = del->prev;
                    if (del->prev->block == block)
                        block->tail = del->prev;
                    /* If the conditional instruction's
                     * prev instruction is in another block,
                     * the conditional must be the only
                     * instruction of the block. Remove it
                     * from the block completely. */
                    else {
                        block->head = NULL;
                        block->tail = NULL;
                    }
                }
                /* If the conditional instruction
                 * does not have a prev pointer,
                 * it must be the only instruction
                 * of the block. Remove it from the
                 * block completely. */
                else {
                    block->head = NULL;
                    block->tail = NULL;
                }

                /* The block is now either empty
                 * or has a new tail. Push the block
                 * to the worklist. */
                worklist.push(block);

                /* If the deleted instruction is the
                 * destination of a jump instruction,
                 * update the jump destination.
                 * In this case, the deleted instruction
                 * must have been the head of the block,
                 * and the updated block must now be empty. */
                if (block->head) continue;

                for (p in block->predecessors) {
                    if (!p->tail) continue;
                    if (p->tail->type == JUMP) {
                        p->tail->instruction.jump.jump_des = next;
                    }
                    else if (
                        p->tail->type == CJUMP
                        && p->tail->instruction.cjump.jump_des == del
                    ) {
                        p->tail->instruction.cjump.jump_des = next;
                    }
                }

                continue;
            }

            /* If the conditional instruction is at
             * the head of the block, following
             * an unconditional jump instruction,
             * replace the unconditional jump
             * with the same conditional jump,
             * and add an unconditional jump block
             * where the jump destination is set
             * to the next instruction of the
             * conditional instruction. */
            if (block->head != block->tail) continue;

            for (p in block->predecessors) {
                if (p->tail && p->tail->type == JUMP) {
                    /* Create a new unconditional jump instruction */
                    Instruction *jump_ins = new_instruction();
                    jump_ins->instruction.jump.jump_des = next;

                    /* Replace the old unconditional jump with
                     * a new conditional jump. */
                    Instruction *cjump_ins = p->tail;
                    Instruction *cjump_prev = p->tail->prev;
                    /* TODO - recheck after set on instruction data structure */
                    cjump_ins->instruction.cjump.operand1 =
                        block->tail->instruction.cjump.operand1;
                    cjump_ins->instruction.cjump.operand2 =
                        block->tail->instruction.cjump.operand2;
                    cjump_ins->instruction.cjump.operator =
                        block->tail->instruction.cjump.operator;
                    cjump_ins->instruction.cjump.jump_des = jump_des;

                    cjump_ins->prev = cjump_prev;
                    cjump_ins->block = p;
                    cjump_ins->next = jump_ins;
                    jump_ins->prev = cjump_ins;

                    /* Create a new block for the jump instruction */
                    Block *jump_block = newblock();
                    jump_ins->block = jump_block;

                    p->successors.add(jump_block);
                    jump_block->predecessors.add(p);
                    jump_block->successors.add(next->block);
                    next->block->predecessors.add(jump_block);

                    /* Add the modified blocks to worklist;
                     * add the new block to reached set. */
                    worklist.add(p);
                    worklist.add(jump_block);
                }
            }
        }

        /* Assess opportunity to eliminate redundant jumps
         * when the tail of the block is an unconditional jump. */
        else if (block->tail->type == JUMP) {
            Instruction *jump_des = block->tail->instruction.jump.jump_des;

            /* Skip if the unconditional jump is in a
             * self-recursive loop. */
            if (jump_des == block->tail) continue;

            /* If the block is succeeded by a successor
             * with only 1 predecessor, remove the
             * unconditional jump. */
            Instruction *s = jump_des->block;
            if (s->predecessors.size() == 1) {
                Instruction *del = block->tail;
                del->block = NULL;

                if (del->prev) {
                    del->prev->next = jump_des;
                    jump_des->prev = del->prev;

                    if (del->prev->block == block)
                        block->tail = del->prev;
                    /* If the unconditional jump's
                     * prev instruction is in another block,
                     * the jump must be the only
                     * instruction of the block. Remove it
                     * from the block completely. */
                    else {
                        block->head = NULL;
                        block->tail = NULL;
                    }
                }
                /* If the unconditional jump
                 * does not have a prev pointer,
                 * it must be the only instruction
                 * of the block. Remove it from the
                 * block completely. */
                else {
                    block->head = NULL;
                    block->tail = NULL;
                }

                /* The block is now either empty
                 * or has a new tail. Push the block
                 * to the worklist. */
                worklist.push(block);

                /* If the deleted instruction is the
                 * destination of a jump instruction,
                 * update the jump destination.
                 * In this case, the deleted instruction
                 * must have been the head of the block,
                 * and the updated block must now be empty. */
                if (block->head) continue;

                for (p in block->predecessors) {
                    if (!p->tail) continue;
                    if (p->tail->type == JUMP) {
                        p->tail->instruction.jump.jump_des = jump_des;
                    }
                    else if (
                        p->tail->type == CJUMP
                        && p->tail->instruction.cjump.jump_des == del
                    ) {
                        p->tail->instruction.cjump.jump_des = jump_des;
                    }
                }

                continue;
            }

            /* If the unconditional instruction is at
             * the head of the block, succeeded by
             * a conditional or unconditional jump,
             * update the jump destination of the
             * preceding instructions to avoid double
             * jumps. */
            if (block->head != block->tail) continue;

            Array<Block *> p_to_remove;

            for (p in block->predecessors) {
                if (!p->tail) continue;
                /* If preceded by an unconditional jump,
                 * update the unconditional jump destination
                 * and update linkage. */
                if (p->tail->type == JUMP) {
                    p->tail->instruction.jump.jump_des = jump_des;
                    p->successors.remove(block);
                    p->successors.add(s);
                    block->predecessors.remove(p);
                    s->precessors.add(p);

                    worklist.add(p);
                    worklist.add(s);
                }
                /* If preceded by a conditional jump... */
                else if (p->tail->type == CJUMP) {
                    Instruction *cond_jump_des = p->tail->instruction.cjump.jump_des;
                    Instruction *cond_next = p->tail->next;

                    /* If the jump destination and the next
                     * instruction of the conditional jump
                     * are the same, remove its linkage
                     * to the conditional jump and reprocess
                     * it later for further optimizations. */
                    if (cond_jump_des == cond_next) {
                        p->tail->instruction.cjump.jump_des = jump_des;
                        p->tail->next = jump_des;
                        jump_des->prev = p->tail;
                        block->head->prev = NULL;

                        p->successors.remove(block);
                        p->successors.add(s);
                        p_to_remove.add(p);
                        s->predecessors.add(p);

                        worklist.add(p);
                        worklist.add(s);
                    }

                    /* If the jump destination of the conditional
                     * jump is the unconditional jump, update the
                     * jump destination and update linkage */
                    else if (cond_jump_des == block->head) {
                        p->tail->instruction.cjump.jump_des = jump_des;

                        p->successors.remove(block);
                        p->successors.add(s);
                        p_to_remove.add(p);
                        s->predecessors.add(p);

                        worklist.add(p);
                        worklist.add(s);
                    }

                    /* If the next instruction of the conditional
                     * is the unconditional jump, and the
                     * jump destination of the unconditional jump
                     * equals the jump destination of the
                     * conditional jump (i.e., if cond goto C else goto C),
                     * change the conditional jump into an
                     * unconditional jump and update linkage. */
                    else if (
                        cond_jump_des == jump_des
                        && cond_next == block->head
                    ) {
                        Instruction *cond_prev = p->tail->prev;
                        p->tail->type = JUMP;
                        p->tail->instruction.jump.jump_des = jump_des;
                        p->next = NULL;
                        p->prev = cond_prev;
                        block->head->prev = NULL;

                        p->successors.remove(block);
                        p->successors.add(s);
                        p_to_remove.add(p);
                        s->predecessors.add(p);

                        worklist.add(p);
                        worklist.add(s);
                    }
                }
            }

            for (p in p_to_remove) {
                block->predecessors.remove(p);
            }
        }

        /* TODO: START HERE If the block has only 1 successor,
         * and the successor has only 1 predecessor, 
         * and successor is not exit, merge the
         * successor into the block. */
        else if (block->successors.size() == 1) {
            Block *s = block->successors[0];
            if (s->predecessors.size() != 1) continue;
            if (s->type == EXIT) continue;
            
        }

        /* TODO: DELETE If a block has an unconditional jump as its
         * last instruction (and therefore has only 1
         * successor), and its successor has only 1
         * predecessor, merge the successor into the block. */
        else if (
            block->tail
            && block->tail->type == JUMP
            && block not in block->predecessors
            && block not in block->successors
        ) {
            /* If the block has more than 1 successor,
             * throw an error */
            if (block->successors.size() > 1) {
                /* TODO: error */
            }

            Block *s = block->successors[0];
            block->sucessors.remove(s);

            /* If the successor has multiple predecessors
             * or is the EXIT block, do nothing */
            if (s->predecessors.size() > 1 || s->type == EXIT)
                continue;

            /* Update the block attribute of each instruction
             * in the successor block, and set up linkage. */
            if (s->head) {
                Instruction *ins = s->head;
                while (ins && ins->block == s) {
                    ins->block = block;
                    ins = ins->next;
                }
                /* Note that the block must have multiple
                 * instructions, since the single unconditional
                 * jump case is already taken care of. Therefore,
                 * block->tail->prev cannot be NULL. */
                Instruction *ptail = block->tail->prev;
                ptail->next = s->head;
                s->head->prev = ptail;
            }

            /* Collapse the stale successor block
             * and push the current block
             * back to the worklist. */
            for (g in s->successors) {
                g->predecessors.remove(s);
                g->predecessors.add(block);
                block->successors.add(g);
            }

            worklist.add(block);

            /* Add the stale successor to garbage,
             * because the stale successor may be
             * already on the worklist. */
            garbage.add(s);
        }
    }

    return b_entry;
}
