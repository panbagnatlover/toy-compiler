/* Instruction as doubly linked list. */
#ifndef H_INSTRUCTION
#define H_INSTRUCTION

#include <stdlib.h>
#include <stdbool.h>
#include "dll.h"
#include "operand.h"
#include "vreg.h"

typedef struct Block;

typedef enum {
    /* IR instruction types */

    /* Machine instruction types */
    INST_ADD,
    INST_SUB,
    INST_LDR,
    INST_STR
} InstType;

/* TODO: decide whether to keep
 * machine instruction in the same type
 * or separate it out in its own type.
 * Leaning separating it out in its own type. */
/* TODO: MInst needs to have predecessors and successors. */
/* Extension of DDLElem. */
typedef struct {
    DLLElem *next;
    DLLElem *prev;

    size_t ind;
    InstType type;

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

    Block *block;
} Inst;

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
