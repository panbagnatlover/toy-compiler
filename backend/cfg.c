#include <stdlib.h>
#include <stdbool.h>
#include "block.h"

/* TODO: at some point, build dominator tree and identify
 * loop head. Is Loop head <=> block dominates a predecessor. */

/* TODO: input instruction array should be a fixed pool.
 * Instruction cjump and jump have NULL next. Their
 * successors have NULL prev. */

/* TODO: worth making worklist a set queue? */

/* TODO p2: set doubly links as indexed access as opposed
 * to pointer access. */

Block *buildCFG(ArrayPool *instructions) {

    Block *b_entry = block();
    Block *b_exit = block();

    /* Map each label to its target instruction. */
    Dict<Label *, Instruction *>map;
    Instruction *target = i_exit;
    for (int i = instructions.size() - 1; i >= 0; i--) {
        if (instructions[i].type == LABEL) {
            map[instructions[i].label] = target;
        }
        else target = instructions[i];
    }

    /* Mark leaders and create leader blocks.
     * A leader is an instruction that either follows
     * a jump instruction or is the target of a jump
     * instruction. */
    Instruction *prev = NULL;
    Instruction *curr = NULL;
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i].type == LABEL) continue;
        curr = instructions[i];

        /* Mark the first instruction as a leader. */
        if (!prev) {
            curr->is_leader = true;
            curr->block = newblock();
            continue;
        }

        /* If the previous instruction is a jump
         * or function call, mark the current 
         * instruction as a leader. */
        if (
            prev->type == JUMP
            || prev->type == COND
            || prev->type == FUNC
        ) {
            curr->is_leader = true;
            if (!curr->block) curr->block = newblock();
            curr->block->head = curr;
        }

        /* Mark the destination of a jump as a leader.
         * Temporarily set the destination of a jump
         * to the location of the jumped-to instruction. */
        Instruction *des;
        switch (curr->type) {
            case JUMP:
                des = map[(Label *) (curr->instruction.jump.des)];
                des->is_leader = true;
                if (!des->block) des->block = newblock();
                des->block->head = des;
                curr->instruction.jump.des = (void *) des;
                break;
            case COND:
                des = map[(Label *) (curr->instruction.cond.des)];
                des->is_leader = true;
                if (!des->block) des->block = newblock();
                des->block->head = des;
                curr->instruction.cond.des = (void *) des;
                break;
            default:
        }

        prev = curr;
    }

    /* Early exit if no instructions exist */
    if (!curr) {
        b_entry->bl0 = b_exit;
        b_exit.predecessors.add(b_entry);
        /* TODO - free memory */
        return b_entry;
    }

    /* Assign block for each instruction and
     * set up block linkage. */
    curr = NULL;
    prev = NULL;
    Block *block = NULL;
    for (int i = 0; i < instructions.size(); i++) {
        if (instructions[i]->type == LABEL) continue;

        curr = instructions[i];

        /* Assign block to each instruction. */
        if (curr->is_leader) block = curr->block;
        else {
            curr->block = block;
            curr->prev = prev;
            prev->next = curr;
        }

        /* Link the first block to the entry block. */
        if (curr->is_leader && !prev) {
            b_entry->bl0 = curr->block;
            curr->block.predecessors.add(b_entry);
        }

        /* Set up block linkage at branch instructions. */
        if (curr->type == JUMP) {
            block->tail = curr;
            Instruction *des = (Instruction *) curr->instruction.jump.des;
            block->bl0 = des->block;
            des->block.predecessors.add(block);
        }
        else if (curr->type == COND) {
            block->tail = curr;
            Instruction *des = (Instruction *) curr->instruction.cond.des;
            block->bl1 = des->block;
            des->block.predecessors.add(block);
        }

        /* Set up block linkage at terminating instructions. */
        if (curr->is_leader && prev && prev->type != JUMP) {
            prev->block->tail = instructions[i];
            prev->block->bl0 = block;
            block->predecessors.add(prev->block);
        }

        prev = curr;
    }

    /* Link last block to exit. */
    if (curr->type != JUMP) {
        curr->block->tail = curr;
        curr->block->bl0 = b_exit;
        b_exit->predecessors.add(curr->block);
    }
}

void _postorder(Block *block, Set<Block*> *visited, Array *array) {
    visited.add(block);

    if (block->bl0 && !visited.contains(block->bl0))
        _postorder(block->bl0, visited, array);
    
    if (block->bl1 && !visited.contains(block->bl1))
        _postorder(block->bl1, visited, array);

    array.add(block);
}

/* Return array of blocks in reverse postorder. */
Array *orderCFG(Block *entry) {
    Array *block_array;
    Set<Block *> visited;

    _postorder(entry, &visited, block_array);

    if (exit is not in visited) {
        /* TODO: program warning: program does not halt */
    }
    return reverse(block_array);
}

/* CFG simplification: analyze the reachable blocks
 * and repeatedly eliminate redundant jumps
 * and collapse redundant blocks. At every iteration,
 * push modified blocks back to the worklist. Repeat
 * until worklist is empty. */
void simplifyCFG(Block *b_entry) {
    Stack<Block *> worklist;
    Set<Block *> added;

    /* Add reachable blocks other than entry and exit
     * to worklist. */
    Array<Block *> *reached = orderCFG(b_entry);
    for (block in reached) {
        if (block != b_entry && block->type != EXIT)
            worklist.push(block);
            added.add(block);
    }

    while (!worklist.empty()) {
        Block *block = worklist.top();
        worklist.pop();
        added.remove(block);

        /* Skip entry and exit blocks. */
        if (block == b_entry || block == b_exit) continue;

        /* Skip unreached blocks. */
        if (!block->predecessors) continue;

        /* A block cannot be empty; otherwise,
         * a structural issue has occurred. */
        if (!block->head) {
            /* TODO - error */
        }

        /* Remove blocks with single jump instructions. */
        if (
            block->head
            && (
                block->head->type == JUMP
                /* Also take into account conditionals
                 * where bl0 == bl1. */
                || (
                    block->head->type == COND
                    && block->bl0 == block->bl1
                )
            )
            /* Ensure the block is not self-recursive. */
            && block->bl0 != block
            && block->bl1 != block
        ) {
            Block *s = block->bl0;
            s->predecessors.remove(block);

            for (p in block->predecessors) {
                if (p->bl0 == block) {
                    p->bl0 = s;
                    s->predecessors.add(p);
                }
                if (p->bl1 == block) {
                    p->bl1 = s;
                    s->predecessors.add(p);
                }
                if (!added.contains(p)) {
                    worklist.push(p);
                    added.add(p);
                }
            }
        }

        /* If a block's terminator is an
         * unconditional jump, remove the instruction. */
        else if (
            block->tail
            && (
                block->tail->type == JUMP
                /* Also take into account conditionals
                 * where bl0 == bl1. */
                || (
                    block->tail->type == COND
                    && block->bl0 == block->bl1
                )
            )
            /* Ensure the block is not self-recursive. */
            && block->bl0 != block
            && block->bl1 != block
        ) {
            Instruction *del = block->tail;
            del->block = NULL;
            del->prev->next = NULL;
            block->tail = del->prev;

            /* Update branching. */
            block->bl1 = NULL;

            worklist.push(block);
            added.add(block);
        }

        /* If a block has one successor block,
         * which has one predecessor block, merge
         * the two blocks. */
        else if (
            && !block->bl1
            && block->bl0 != b_exit
            && block->bl0->predecessors.size() == 1
        ) {
            Block *s = block->bl0;
            /* Merge successor instructions to the
             * current block. */
            Instruction *next = s->head;
            Instruction *prev = block->tail;
            if (next) {
                prev->next = next;
                next->prev = prev;
            }
            while (next) {
                next->block = block;
                prev = prev->next;
                next = next->next;
            }
            block->tail = prev;

            /* Update linkage. */
            s->predecessors.reset();
            block->bl0 = s->bl0;
            block->bl1 = s->bl1;
            s->bl0->predecessors.remove(s);
            s->bl0->predecessors.add(block);
            s->bl1->predecessors.remove(s);
            s->bl0->predecessors.add(block);

            worklist.add(block);
            added.add(block);
        }
}

/* Flatten CFG back to array of instructions. */
void flattenCFG(Block *b_entry) {

    /* TODO - assign every block a priority score somehow */

    /* TODO - handle exit */

    worklist.push(b_entry);

    while (!worklist.empty()) {
        Block *block = worklist.pop();
        emit(block without conditional);
        if (!block->bl1) {
            if (block->bl0 is emitted) {
                emit(GOTO bl0);
            }
            else worklist.push(block->bl0);
        }
        else if (block->bl0 and block->bl1 both emitted) {
                emit(block->tail);
                emit(GOTO bl0);
        }
        else if (block->bl0 is emitted) {
            reverse bl0 and bl1;
            emit(block->tail);
            worklist.push(block->bl0);
        }
        else if (block->bl1 is emitted) {
            emit(block->tail);
            worklist.push(block->bl0);
        }
        else if (bl0 priority >= bl1) {
            emit(block->tail);
            worklist.push(block->bl1);
            worklist.push(block->bl0);
        }
        else {
            reverse bl0 and bl1;
            emit(block->tail);
            worklist.push(block->bl1);
            worklist.push(block->bl0);
        }
    }
}
