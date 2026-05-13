/* Three-address code */

#ifndef H_3AC
#define H_3AC

#include <stdbool.h>
#include "operand.h"
#include "context.h"
#include "node.h"
#include "label.h"
#include "error.h"

void emit_quad(Operand temp, Op op, Operand a, Operand b);
void emit_unary(Operand temp, Unary op, Operand a);
void emit_assign(Operand l, Operand a);
void emit_assign_acc(Operand l, Operand a, Operand r);
void emit_if_rel(Rel op, Operand a, Operand b, Label *label);
void emit_if_unary(Unary op, Operand a, Label *label);
void emit_iffalse_rel(Rel op, Operand a, Operand b, Label *label);
void emit_iffalse_unary(Unary op, Operand a, Label *label);
void emit_goto(Label *label, Context *c);
void emit_label(Label *label, bool force, Context *c);

ErrCode stmt_3ac(Node *stmt, Label *s_next, Context *c);
ErrCode cond_3ac(Node *cond, Label *b_true, Label *b_false, Context *c);
Operand expr_3ac(Node *expr, Context *c);

ErrCode logic_3ac(Node *node, Label *b_true, Label *b_false, Context *c);
ErrCode rel_3ac(Node *node, Label *b_true, Label *b_false, Context *c);
ErrCode unary_cond_3ac(Node *node, Label *b_true, Label *b_false, Context *c);
ErrCode bool_cond_3ac(Node *node, Label *b_true, Label *b_false, Context *c);

Operand arith_3ac(Node *node, Context *c);
Operand unary_value_3ac(Node *node, Context *c);
Operand neg_3ac(Node *node, Context *c);
Operand loc_value_3ac(Node *node, Context *c);
Operand loc_offset_3ac(Node *node, Context *c);

ErrCode assign_3ac(Node *node, Context *c);
ErrCode break_3ac(Node *node, Context *c);
ErrCode do_3ac(Node *node, Label *s_next, Context *c);
ErrCode while_3ac(Node *node, Label *s_next, Context *c);
ErrCode if_3ac(Node *node, Label *s_next, Context *c);
ErrCode block_3ac(Node *node, Context *c);

#endif
