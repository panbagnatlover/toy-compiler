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
 * heap allocations.
 * 
 * Each def instance is linked to a use instance.
 * Deleting a used vreg means removing all of its
 * def linkages, and the corresponding defs need to
 * delete the relevant use linkages.
 * */

#ifndef H_VREG
#define H_VREG

#include "dll.h"
#include "inst.h"
#include "var.h"

typedef struct {
    DLLElem *next;
    DLLElem *prev;
    VReg *vreg;
    Use *use;
} Def;

typedef struct {
    DLLElem *next;
    DLLElem *prev;
    VReg *vreg;
    Def *def;
} Use;

typedef enum {
    REG_SP = 0,
    REG_FP,
    REG_X1
} Reg;

typedef struct {
    uint8_t ind;  // Index of a vreg in an instruction. Used for profiling
    Var *var;
    DLL defs;
    DLL uses;
    Reg reg;  // Physical register.
    Inst *inst;
} VReg;

/* Create use def chain between a and b, where a is a def of b. */
void create_use_def_chain(VReg *a, VReg *b);
bool is_def_of(VReg *a, VReg *b);  // Returns true if a defines b

#endif
