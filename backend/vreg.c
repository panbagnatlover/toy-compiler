#include "vreg.h"

void create_use_def_chain(VReg *a, VReg *b) {
    if (is_def_of(a, b)) return;
    /* Create a new use obj, def obj, then link them. */
}
