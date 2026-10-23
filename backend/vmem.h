#ifndef H_VMEMORY
#define H_VMEMORY

/* TODO - need to add stack frame info? */
typedef struct VMemory {
    enum {
        VAR;
        REG_OFFSET;
        CON_OFFSET;
    } type;

    union {
        Variable *var;

        struct {
            VRegister reg; // reg has size info
            Const offset;
            Set<Memory *>must_alias; // TODO - bitset
            Set<Memory *>may_alias;
        };

        struct {
            VRegister reg;
            VRegister offset;
            Set<Memory *>must_alias; // TODO - bitset
            Set<Memory *>may_alias;
        };
    }

    MInst *inst;

    prececessors;
    latencies;  // Changes by instruction
    successors;
};

#endif
