/* Virtual register.
 * Virtual registers are flow-sensitive;
 * therefore, they are instantiated as a different
 * memory object in every instruction.
 * Virtual register instances that represent
 * the same virtual register is linked together
 * by next_use and prev_use. A virtual register
 * on the LHS of an assignment instruction does not
 * have a prev_use. Aside from that, the ordering
 * of the use-chain doesn't matter; in case of a
 * control flow like the following, x1->next_use can
 * be either x2 or x3, where x_i represent the same
 * virtual register (suffix for illustrative purposes):
 * 
 * x1 = 1;
 * if (cond) y = x2 + 1;
 * else y = x3 + 2;
 * 
 * We use a doubly linked list to chain uses together,
 * rather than having a hash set for each virtual
 * register. This is a design decision to limit expensive
 * heap allocations. */

#ifndef H_VREGISTER
#define H_VREGISTER

typedef struct VRegister {
    bool isDef;  // true for def, false for use
    Variable *var;
    Set<VReg *> defs;  // TODO: coalesce into one set
    Set<VReg *> uses;
    MInst *instruction;
};

#endif
