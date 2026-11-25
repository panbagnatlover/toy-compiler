/* Instruction execution resources. */
#ifndef H_INST_PROFILE
#define H_INST_PROFILE

#include "inst.h"
#include "resource.h"

#define MAX_OPERAND 5
#define MAX_MEMORY 3
#define MAX_CYCLES 30

typedef struct {
    // Latencies for each reg / mem in an instruction
    uint8_t reg_latencies[MAX_OPERAND];
    uint8_t mem_latencies[MAX_MEMORY];
    // Total num of cycles for static execution
    uint8_t num_cycles;
    // Static resource profile
    uint8_t res_profile[MAX_CYCLES][NUM_RESOURCE_TYPE];
} InstExecutionProfile;

InstExecutionProfile inst_ldr;
InstExecutionProfile inst_add;

void set_inst_ldr_profile(void) {
    // TODO
}

void set_inst_add_profile(void) {
    // TODO
}

void seed_inst_profile(void) {
    // TODO
}

InstExecutionProfile *get_inst_profile(Inst *inst) {
    switch (inst->type) {
        case INST_LDR:
            break;
        case INST_ADD:
            break;
        default:
    }
}

#endif
