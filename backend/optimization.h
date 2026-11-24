#ifndef H_OPTIMIZATION
#define H_OPTIMIZATION

#include "aliasmap.h"
#include "liveness.h"

void dce(Array *blocks, LivenessContext *liveness_context);
ErrCode mem2reg(Array *blocks, AliasState *alias_state);

#endif
