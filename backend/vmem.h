/* Decision: store pt set in vmem itself. */
#ifndef H_VMEM
#define H_VMEM

#include "var.h"
#include "vreg.h"
#include "aliasmap.h"

/* TODO - need to add stack frame info? */
typedef struct {
    uint8_t ind;  // Index of vmem in an instruction. Used for static execution profiling

    enum {
        VMEM_VAR,  // e.g., define var1 = 0x12345, then [var1]
        VMEM_REG,  // e.g., [SP, r]
        VMEM_CON   // e.g., [SP, #32]
    } type;

    union {
        struct {
            VReg base;
            VReg offset;
        } reg;
        struct {
            VReg reg; // reg has size info
            size_t offset;
        } con;
    } loc;

    /* The # of bytes loaded from or stored to this vmem. */
    uint8_t size;

    Inst *inst;

    PointToSet pt_set;
} VMem;

#endif
