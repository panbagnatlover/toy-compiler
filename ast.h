#ifndef H_AST
#define H_AST

#include <stdlib.h>
#include "error.h"
#include "node.h"
#include "context.h"

Node *expr_analyzer(Node *node, size_t curr, Context *c);
Node *cond_analyzer(Node *node, size_t curr, Context *c);
ErrCode stmt_analyzer(Node *node, size_t curr, Context *c);
Node *id_analyzer(Node *node, size_t curr, Context *c);
Node *arith_analyzer(Node *node, size_t curr, Context *c);
Node *loc_analyzer(Node *node, size_t curr, Context *c);
Node *neg_analyzer(Node *node, size_t curr, Context *c);
Node *not_analyzer(Node *node, size_t curr, Context *c);
Node *rel_analyzer(Node *node, size_t curr, Context *c);
Node *logic_analyzer(Node *node, size_t curr, Context *c);
ErrCode assign_analyzer(Node *node, size_t curr, Context *c);
ErrCode decl_analyzer(Node *node, size_t curr, Context *c);
ErrCode block_analyzer(Node *node, size_t prev_st, Context *c);
ErrCode program_analyzer(Node *root, Context *c);

#endif
