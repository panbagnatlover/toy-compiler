#ifndef H_CFG
#define H_CFG

#include "block.h"
#include "array.h"

Block *buildCFG(ArrayPool *instructions);
Array *orderCFG(Block *entry);
void simplifyCFG(Block *b_entry);
void flattenCFG(Block *b_entry);

#endif
