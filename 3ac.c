/* Three-address code for IR.
 * Emit format:
 * 1. t = a op b, where op can be +, -, *, /, access
 * 2. t = op b, where op can be -, cast, not zero
 * 3. s = a, where s is an ID
 * 4. s[t] = a, where s is an ID
 * 5. if a rel b goto L
 * 6. if unary b goto L
 * 7. iffalse a rel b goto L
 * 8. iffalse unary b goto L
 * 9. goto L
 * 10. L
 * 
 * Error handling:
 * Fail immediately if any error is encountered.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "type.h"
#include "node.h"
#include "symbol.h"
#include "label.h"
#include "operand.h"
#include "context.h"
#include "3ac.h"
#include "utils.h"
#include "log.h"

/* Code emission.
 * Pushes a 3-address code object to an array
 * for backend operations. Also prints a human-readable
 * form.
 */

void print_operand(Operand a) {
    switch (a.id) {
        case CONST:
            switch (a.constant.type_id) {
                case T_INT:
                    printf("%d", a.constant.num);
                    break;
                case T_FLOAT:  
                    printf("%f", a.constant.flo);
                    break;
                case T_BOOL:
                    a.constant.boo ? printf("true") : printf("false");
                    break;
                default: printf("err");
            }
            break;
        case VAR:
            printf("%s", a.symbol->name);
            break;
        case TEMP:
            printf("t%d", a.symbol->ind);
            break;
        default: printf("err");
    }
}

void print_op(Op a) {
    switch (a) {
        case O_ACC: printf("acc"); break;
        case O_PLU: printf("add"); break;
        case O_MIN: printf("sub"); break;
        case O_MUL: printf("mul"); break;
        case O_DIV: printf("div"); break;
        default: printf("err");
    }
}

void print_unary(Unary a) {
    switch (a) {
        case U_NEG:         printf("neg"); break;
        case U_NOT:         printf("not"); break;
        case U_NOT_ZERO:    printf("notzero"); break;
        case U_CAST_INT:    printf("int"); break;
        case U_CAST_FLOAT:  printf("float"); break;
        case U_CAST_BOOL:   printf("bool"); break;
        default:            printf("err");
    }
}

void print_rel(Rel a) {
    switch (a) {
        case R_EQ:  printf("eq"); break;
        case R_NEQ: printf("neq"); break;
        case R_GE:  printf("ge"); break;
        case R_GEQ: printf("geq"); break;
        case R_LE:  printf("le"); break;
        case R_LEQ: printf("leq"); break;
        default:    printf("err");
    }
}

void print_label(Label *l) {
    printf("L%d", l->ind);
}

void emit_quad(Operand temp, Op op, Operand a, Operand b) {
    // TODO: push to 3ac array

    print_operand(temp); printf(" = ");
    print_operand(a); printf(" ");
    print_op(op); printf(" ");
    print_operand(b); printf("\n");
}

void emit_unary(Operand temp, Unary op, Operand a) {
    // TODO

    print_operand(temp); printf(" = ");
    print_unary(op); printf(" ");
    print_operand(a); printf("\n");
}

void emit_assign(Operand l, Operand a) {

    print_operand(l); printf(" = ");
    print_operand(a); printf("\n");
}

void emit_assign_acc(Operand l, Operand a, Operand r) {
    print_operand(l); printf("["); print_operand(a); printf("]");
    printf(" = "); print_operand(r); printf("\n");
}

void emit_if_rel(Rel op, Operand a, Operand b, Label *label) {
    label->status = FILLED;

    printf("if "); print_operand(a); printf(" ");
    print_rel(op); printf(" "); print_operand(b);
    printf(" goto "); print_label(label); printf("\n");
}

void emit_if_unary(Unary op, Operand a, Label *label) {
    label->status = FILLED;

    printf("if "); print_unary(op); printf(" ");
    print_operand(a); printf(" goto "); print_label(label);
    printf("\n");
}

void emit_iffalse_rel(Rel op, Operand a, Operand b, Label *label) {
    label->status = FILLED;

    printf("iffalse "); print_operand(a); printf(" ");
    print_rel(op); printf(" "); print_operand(b);
    printf(" goto "); print_label(label); printf("\n");
}

void emit_iffalse_unary(Unary op, Operand a, Label *label) {
    label->status = FILLED;

    printf("iffalse "); print_unary(op); printf(" ");
    print_operand(a); printf(" goto "); print_label(label);
    printf("\n");
}

/* Force evaluation of a label */
void emit_goto(Label *label, Context *c) {
    label->status = FILLED;
    // TODO: push to 3ac array

    printf("goto "); print_label(label); printf("\n");
}

/* force = whether to force evaluation of a label */
void emit_label(Label *label, bool force, Context *c) {
    /* If label is fall, or label is not evaluated,
     * emit nothing */
    if (!label) return;
    if (!force && label->status == CREATED) return;

    // TODO: push to 3ac array

    print_label(label); printf(":\n");
}

/* Convert a conditional expression to a value.
 * We do so by translating expression B with
 * logic for:
 * if (B) t = true else t = false; return t. */
Operand to_val(Node *cond, Context *c) {
    /* Validate that input conditional node
     * is a value node */
    if (!cond->type) {
        EMIT_SEMANTIC_ERR(
            0,
            "Input node is not a value node."
        );
        c->has_semantic_error = true;
        return err_operand();
    }

    Label *b_true = NULL;
    Label *b_false = newlabel(c); if (!b_false) return err_operand();
    Label *s_next = newlabel(c); if (!s_next) return err_operand();
    ErrCode e = cond_3ac(cond, b_true, b_false, c); if (e) return err_operand();

    /* Create a new boolean temp */
    Symbol *t = newtemp(c->bool_type, c); if (!t) return err_operand();
    Operand t_op = temp_operand(t);

    /* temp = true */
    Operand tr = bool_operand(true);
    emit_assign(t_op, tr);

    emit_goto(s_next, c);
    emit_label(b_false, false, c);

    /* temp = false */
    Operand fa = bool_operand(false);
    emit_assign(t_op, fa);

    emit_label(s_next, false, c);

    return t_op;
}

/* 3ac */

/* Orchestrator for 3-address code generation for statements.*/
ErrCode stmt_3ac(Node *stmt, Label *s_next, Context *c) {
    switch (stmt->node_id) {
        case N_IF:      return if_3ac(stmt, s_next, c);
        case N_WHILE:   return while_3ac(stmt, s_next, c);
        case N_DO:      return do_3ac(stmt, s_next, c);
        case N_BREAK:   return break_3ac(stmt, c);
        case N_ASSIGN:  return assign_3ac(stmt, c);
        case N_BLOCK:   return block_3ac(stmt, c);
        default:
            LOG_DBG("Input node is not a stmt node.");
            return INTERNAL_ERR;
    }
}

/* Orchestrator for 3-address code generation for conditionals. */
ErrCode cond_3ac(Node *cond, Label *b_true, Label *b_false, Context *c) {
    switch (cond->node_id) {
        case N_LOGIC:   return logic_3ac(cond, b_true, b_false, c);
        case N_REL:     return rel_3ac(cond, b_true, b_false, c);
        case N_UNARY:   return unary_cond_3ac(cond, b_true, b_false, c);
        case N_BOOL:    return bool_cond_3ac(cond, b_true, b_false, c);
        default:
            LOG_DBG("Input node is not a conditional node.");
            return INTERNAL_ERR;
    }
}

/* Orchestrator for 3-address code generation for
 * non-conditional expressions. Returns an operand
 * representing the value of the expression. */
Operand expr_3ac(Node *expr, Context *c) {
    /* Validate that expr is a value node. */
    if (!expr->type) {
        LOG_DBG("Input node is not an expr node.");
        return err_operand();
    }

    switch (expr->node_id) {
        case N_ARITH:           return arith_3ac(expr, c);
        case N_LOC:             return loc_value_3ac(expr, c);
        case N_LOGIC || N_REL:  return to_val(expr, c);
        case N_UNARY:           return unary_value_3ac(expr, c);
        case N_BOOL:            return bool_operand(expr->boo.value);
        case N_NUM:             return num_operand(expr->num.value);
        case N_FLO:             return float_operand(expr->flo.value);
        case N_ID:              return var_operand(expr->id.symbol);
        default:
            LOG_DBG("Input node is not an expr node.");
            return err_operand();
    }
}

/* Conditional 3-address code generation for B -> B1 AND/OR B2 */
ErrCode logic_3ac(Node *node, Label *b_true, Label *b_false, Context *c) {
    /* B -> B1 || B2 */
    if (node->logic.op == L_OR) {
        Label *b1_true = b_true ? b_true : newlabel(c);
        if (!b1_true) return INTERNAL_ERR;
        Label *b1_false = NULL;

        ErrCode e = cond_3ac(node->logic.left, b1_true, b1_false, c);
        if (e) return e;
        e = cond_3ac(node->logic.right, b_true, b_false, c);
        if (e) return e;

        if (!b_true) emit_label(b1_true, false, c);
    }
    /* B -> B1 && B2 */
    else {
        Label *b1_true = NULL;
        Label *b1_false = b_false ? b_false : newlabel(c);
        if (!b1_false) return INTERNAL_ERR;

        ErrCode e = cond_3ac(node->logic.left, b1_true, b1_false, c);
        if (e) return e;
        e = cond_3ac(node->logic.right, b_true, b_false, c);
        if (e) return e;

        if (!b_false) emit_label(b1_false, false, c);
    }

    return NO_ERR;
}

/* Conditional 3-address code generation for B -> B1 rel B2 */
ErrCode rel_3ac(Node *node, Label *b_true, Label *b_false, Context *c) {
    Operand t1 = expr_3ac(node->rel.left, c);
    if (t1.id == ERR_OPERAND) return INTERNAL_ERR;

    Operand t2 = expr_3ac(node->rel.right, c);
    if (t2.id == ERR_OPERAND) return INTERNAL_ERR;

    Rel op = node->rel.op;

    if (b_true && b_false) {
        emit_if_rel(op, t1, t2, b_true);
        emit_goto(b_false, c);
    }
    else if (b_true) emit_if_rel(op, t1, t2, b_true);
    else if (b_false) emit_iffalse_rel(op, t1, t2, b_false);

    return NO_ERR;
}

/* Conditional 3-address code generation for B -> UNARY B1 */
ErrCode unary_cond_3ac(Node *node, Label *b_true, Label *b_false, Context *c) {
    /* B -> !B1 */
    if (node->unary.op == U_NOT) {
        return cond_3ac(node->unary.child, b_false, b_true, c);
    }
    /* B -> NOTZERO(B1) */
    else if (node->unary.op == U_NOT_ZERO) {
        Operand t = expr_3ac(node->unary.child, c);
        if (t.id == ERR_OPERAND) return INTERNAL_ERR;

        if (b_true && b_false) {
            emit_if_unary(U_NOT_ZERO, t, b_true);
            emit_goto(b_false, c);
        }
        else if (b_true) emit_if_unary(U_NOT_ZERO, t, b_true);
        else if (b_false) emit_iffalse_unary(U_NOT_ZERO, t, b_false);
    }
    else {
        LOG_DBG("Non-conditional unary op.");
        return INTERNAL_ERR;
    }

    return NO_ERR;
}

/* Conditional 3-address code generation for B -> true | false */
ErrCode bool_cond_3ac(Node *node, Label *b_true, Label *b_false, Context *c) {
    /* B -> true */
    if (b_true && node->boo.value) {
        emit_goto(b_true, c);
    }
    /* B -> false */
    else if (b_false && !node->boo.value) {
        emit_goto(b_false, c);
    }

    return NO_ERR;
}

Operand arith_3ac(Node *node, Context *c) {
    Operand l = expr_3ac(node->arith.left, c);
    Operand r = expr_3ac(node->arith.right, c);

    Symbol *t = newtemp(node->type, c); if (!t) return err_operand();
    Operand t_op = temp_operand(t);

    emit_quad(t_op, node->arith.op, l, r);
    return t_op;
}

Operand unary_value_3ac(Node *node, Context *c) {
    if (
        node->unary.op != U_NEG &&
        node->unary.op != U_CAST_BOOL &&
        node->unary.op != U_CAST_FLOAT &&
        node->unary.op != U_CAST_INT
    ) {
        LOG_DBG("Not a value expression.");
        return err_operand();
    }

    Symbol *t = newtemp(node->type, c); if (!t) return err_operand();

    Operand t_op = temp_operand(t);
    Operand r = expr_3ac(node->unary.child, c);
    if (r.id == ERR_OPERAND) return r;

    emit_unary(t_op, node->unary.op, r);
    return t_op;
}

Operand neg_3ac(Node *node, Context *c) {
    Operand r = expr_3ac(node->unary.child, c);
    if (r.id == ERR_OPERAND) return r;

    Symbol *t = newtemp(node->type, c); if (!t) return err_operand();
    Operand t_op = temp_operand(t);

    emit_unary(t_op, U_NEG, r);
    return t_op;
}

/* Returns an operand representing the value
 * at the calculated offset from base memory.
 * Invoked by expr_3ac orchestrator when rvalue
 * is a LOC node. */
Operand loc_value_3ac(Node *node, Context *c) {
    Operand offset = loc_offset_3ac(node, c);
    if (offset.id == ERR_OPERAND) return offset;

    Node *base = node->loc.base;
    Operand base_op = var_operand(base->id.symbol);

    Symbol *t = newtemp(node->type, c); if (!t) return err_operand();
    Operand t_op = temp_operand(t);

    emit_quad(t_op, O_ACC, base_op, offset);
    return t_op;
}

/* Returns an operand representing the offset from
 * base memory. Invoked by ASSIGN statement when
 * lvalue is a LOC node. */
Operand loc_offset_3ac(Node *node, Context *c) {
    Node *left = node->loc.left;
    Node *ind = node->loc.ind;

    /* Get the current offset */
    Type *type = left->type->elem;
    Operand width = num_operand(type->width);
    Operand unit = expr_3ac(ind, c);
    if (unit.id == ERR_OPERAND) return unit;

    /* If left node is the base address, offset
     * is index times width of the top-level element */
    if (left->node_id == N_ID) {
        Symbol *t = newtemp(c->int_type, c); if (!t) return err_operand();
        Operand t_op = temp_operand(t);
        emit_quad(t_op, O_MUL, width, unit);
        return t_op;
    }

    /* Otherwise, offset is the prior offset address
     * plus index x width of the current element */
    Operand base = loc_offset_3ac(left, c);
    if (base.id == ERR_OPERAND) return base;

    Symbol *t1 = newtemp(c->int_type, c); if (!t1) return err_operand();
    Operand t1_op = temp_operand(t1);
    emit_quad(t1_op, O_MUL, width, unit);

    Symbol *t2 = newtemp(c->int_type, c); if (!t2) return err_operand();
    Operand t2_op = temp_operand(t2);
    emit_quad(t2_op, O_PLU, base, t1_op);
    return t2_op;
}

ErrCode assign_3ac(Node *node, Context *c) {
    Operand rvalue = expr_3ac(node->assign.rvalue, c);
    if (rvalue.id == ERR_OPERAND) return INTERNAL_ERR;

    Node *left = node->assign.lvalue;
    /* If left node represents a base variable,
     * assign the rvalue to it */
    if (left->node_id == N_ID) {
        Operand lvalue = var_operand(left->id.symbol);
        emit_assign(lvalue, rvalue);
    }
    /* If left node represents array access,
     * run loc_offset_3ac to get the offset operand */
    else {
        Operand offset = loc_offset_3ac(left, c);
        if (offset.id == ERR_OPERAND) return INTERNAL_ERR;

        Node *base = left->loc.base;
        Operand base_op = var_operand(base->id.symbol);
        emit_assign_acc(base_op, offset, rvalue);
    }

    return NO_ERR;
}

/* Goto the statement after the current loop. */
ErrCode break_3ac(Node *node, Context *c) {
    if (!c->exit_stack->size) {
        EMIT_SEMANTIC_ERR(
            0,
            "No outer loop to break from."
        );
        c->has_semantic_error = true;
        return BUG_DETECTED;
    }
    size_t top_ind = c->exit_stack->size - 1;

    emit_goto(((Label **) c->exit_stack->arr)[top_ind], c);

    return NO_ERR;
}

ErrCode do_3ac(Node *node, Label *s_next, Context *c) {
    Node *cond = node->s_do.cond;
    Node *stmt = node->s_do.stmt;

    /* Mark the beginning of loop */
    Label *begin = newlabel(c); if (!begin) return INTERNAL_ERR;
    emit_label(begin, true, c);

    /* Push s_next to exit stack */
    ErrCode e = push_array(&s_next, c->exit_stack);
    if (e) {
        LOG_ERR("Failed to push label to exit stack.");
        return e;
    }

    Label *s1_next = newlabel(c); if (!s1_next) return INTERNAL_ERR;

    e = stmt_3ac(stmt, s1_next, c); if (e) return e;

    emit_label(s1_next, false, c);

    Label *b_true = begin;
    Label *b_false = NULL;
    e = cond_3ac(cond, b_true, b_false, c); if (e) return e;

    /* Pop s_next from the exit label stack */
    e = pop_array(c->exit_stack);
    if (e) {
        LOG_ERR("Failed to pop the top label from exit stack.");
        return e;
    }

    return NO_ERR;
}

/* S -> if (B) {do S1 while B1} */
ErrCode while_3ac(Node *node, Label *s_next, Context *c) {
    Node *cond = node->s_while.cond;
    Node *stmt = node->s_while.stmt;

    Label *b_true = NULL;
    Label *b_false = s_next;
    ErrCode e = cond_3ac(cond, b_true, b_false, c); if (e) return e;

    /* Mark the beginning of loop */
    Label *begin = newlabel(c); if (!begin) return INTERNAL_ERR;
    emit_label(begin, true, c);

    /* Push s_next to exit stack */
    e = push_array(&s_next, c->exit_stack);
    if (e) {
        LOG_ERR("Failed to push label to exit stack.");
        return e;
    }

    Label *s1_next = newlabel(c); if (!s1_next) return INTERNAL_ERR;
    e = stmt_3ac(stmt, s1_next, c); if (e) return e;
    emit_label(s1_next, false, c);

    Label *b1_true = begin;
    Label *b1_false = NULL;
    e = cond_3ac(cond, b1_true, b1_false, c); if (e) return e;

    /* Pop s_next from the exit label stack */
    e = pop_array(c->exit_stack);
    if (e) {
        LOG_ERR("Failed to pop the top label from exit stack.");
        return e;
    }

    return NO_ERR;
}

ErrCode if_3ac(Node *node, Label *s_next, Context *c) {
    Node *cond = node->s_if.cond;
    Node *stmt = node->s_if.stmt;
    Node *estmt = node->s_if.estmt;

    /* S -> if (B) S1 */
    if (!estmt) {
        Label *b_true = NULL;
        Label *b_false = s_next;

        ErrCode e = cond_3ac(cond, b_true, b_false, c); if (e) return e;
        e = stmt_3ac(stmt, s_next, c); if (e) return e;
    }

    /* S -> if (B) S1 else S2 */
    else {
        Label *b_true = newlabel(c); if (!b_true) return INTERNAL_ERR;
        Label *b_false = newlabel(c); if (!b_false) return INTERNAL_ERR;

        ErrCode e = cond_3ac(cond, b_true, b_false, c); if (e) return e;
        e = stmt_3ac(stmt, s_next, c); if (e) return e;

        emit_goto(s_next, c);
        emit_label(b_false, false, c);

        e = stmt_3ac(estmt, s_next, c); if (e) return e;
    }

    return NO_ERR;
}

ErrCode block_3ac(Node *node, Context *c) {
    /* Skip declarations since they don't need translations.
     * Translate each statement. */
    Array *stmts = node->block.stmts;
    for (int i = 0; i < stmts->size; i++) {
        Label *s_next = newlabel(c); if (!s_next) return INTERNAL_ERR;

        ErrCode e = stmt_3ac(((Node **) stmts->arr)[i], s_next, c);
        if (e) return e;

        emit_label(s_next, false, c);
    }

    return NO_ERR;
}
