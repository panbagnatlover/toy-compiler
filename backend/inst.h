/* Instruction as doubly linked list. */
#ifndef H_INSTRUCTION
#define H_INSTRUCTION

#include <stdlib.h>
#include <stdbool.h>

typedef struct Block;

/* TODO: decide whether to keep
 * machine instruction in the same type
 * or separate it out in its own type.
 * Leaning separating it out in its own type. */
/* TODO: MInst needs to have predecessors and successors. */
typedef struct Instruction {
    enum {
       LABEL,
       DUMMY_ENTRY,
       DUMMY_EXIT,
       /* IR instruction types */
       IR_COND,
       IR_JUMP,
       IR_FUNC,
       ...
       /* Machine instruction types */
       M_blabla
    } id;
    /* TODO: Operands, Op */

    union {
        struct {
            void *des; /* Not used after block construction. */
        } jump;

        struct {
            Operand operand1;
            Operand operand2;
            Operator operator;
            void *des; /* Not used after block construction. */
        } cond;
    } instruction;

    bool is_leader;

    struct Instruction *prev;
    struct Instruction *next;

    struct Block *block;
};

typedef struct MInst {
    /* TODO - same as instruction */
    /* TODO - for each operand, latency */
    predecessors;
    successors;
    latencies;
    int height;
    size_t index;

    /* 2D array representing units of resources needed 
     * to execute this instruction. Row
     * represents time step, and column represents
     * types of resources. */
    int resources[MAX_TIME_STEP][RESOURCES];
};

#endif
