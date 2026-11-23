/* Decision: store pt set in vmem itself. */
#ifndef H_VMEM
#define H_VMEM

#include "var.h"
#include "aliasmap.h"

/* TODO - need to add stack frame info? */
typedef struct {
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

    Inst *inst;

    PointToSet pt_set;
} VMem;

#endif
