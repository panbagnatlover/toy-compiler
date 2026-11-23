#ifndef H_DOMINATOR
#define H_DOMINATOR

#include "inst.h"
#include "block.h"

bool dominate_block(Block *a, Block *b);

bool dominate(Inst *a, Inst *b);

#endif
