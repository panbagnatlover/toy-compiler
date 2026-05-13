/* Sementic analyzer.
 * Type checking, type coersion, scope checking,
 * control flow identification, and constant folding.
 * - Type checking populates each expression node
 * with a type and check type compatibility in assignment.
 * - Type coersion adds a CAST node to coerce a node
 * into another type, or replaces node.type directly.
 * - Scope checking involves creating symbols for
 * each ID, push to the symbol table in scope,
 * and validate their scope during assignment. 
 * - Control flow identification involves identifying
 * whether a boolean node is an expression or
 * a conditional statement.
 * - Constant folding involves replace a non-constant
 * node into a constant node where it makes sense.
 * 
 * Types of nodes:
 * Non-conditional node = expression node that cannot represent
 * a conditional.
 * Conditional node = expression node that can represent
 * a conditional.
 * Expression node = union of non-conditional and conditional nodes.
 * Value node = nodes with a defined value. Must have a type.
 * 
 * Error handling behavior:
 * All semantic errors result in hard failure
 * except scope checking. Any program with syntax
 * or semantic errors will not go on to the next stage.
 */

#include <stdio.h>
#include "ast.h"
#include "symbol.h"
#include "context.h"
#include "utils.h"
#include "log.h"

/* Identify whether an expression node can
 * represent a conditional node */

bool is_cond(Node *n) {
    return n->node_id == N_LOGIC ||
        n->node_id == N_REL ||
        n->node_id == N_BOOL ||
        (n->node_id == N_UNARY && n->unary.op == U_NOT);
}

bool is_not_cond(Node *n) {
    return n->node_id == N_ARITH ||
        n->node_id == N_ID ||
        n->node_id == N_LOC ||
        n->node_id == N_NUM ||
        n->node_id == N_FLO ||
        (n->node_id == N_UNARY && n->unary.op == U_NEG);
}

/* Identify if an expression node represents a constant */

bool is_const(Node *n) {
    return n->node_id == N_NUM ||
        n->node_id == N_FLO ||
        n->node_id == N_BOOL;
}

/* Constant folding helpers */

Node *arith_int(Node *p, Node *a, Node *b, Context *c) {
    int l = a->num.value;
    int r = b->num.value;
    int final;

    switch (p->arith.op) {
        case O_PLU: final = l + r; break;
        case O_MIN: final = l - r; break;
        case O_MUL: final = l * r; break;
        case O_DIV: final = l / r; break;
        default:
            LOG_ERR("Unrecognized arith op.");
            return NULL;
    }

    return num_node(final, p->line, c);
}

Node *arith_float(Node *p, Node *a, Node *b, Context *c) {
    float l = a->flo.value;
    float r = b->flo.value;
    float final;

    switch (p->arith.op) {
        case O_PLU: final = l + r; break;
        case O_MIN: final = l - r; break;
        case O_MUL: final = l * r; break;
        case O_DIV: final = l / r; break;
        default:
            LOG_ERR("Unrecognized arith op.");
            return NULL;
    }

    return float_node(final, p->line, c);
}

Node *arith_bool(Node *p, Node *a, Node *b, Context *c) {
    int l = a->boo.value ? 1 : 0;
    int r = b->boo.value ? 1 : 0;
    int final;

    switch (p->arith.op) {
        case O_PLU: final = l + r; break;
        case O_MIN: final = l - r; break;
        case O_MUL: final = l * r; break;
        case O_DIV: final = l / r; break;
        default:
            LOG_ERR("Unrecognized arith op.");
            return NULL;
    }

    return num_node(final, p->line, c);
}

Node *rel_int(Node *p, Node *a, Node *b, Context *c) {
    int l = a->num.value;
    int r = b->num.value;
    bool final;

    switch (p->rel.op) {
        case R_EQ:  final = l == r; break;
        case R_NEQ: final = l != r; break;
        case R_GE:  final = l > r; break;
        case R_GEQ: final = l >= r; break;
        case R_LE:  final = l < r; break;
        case R_LEQ: final = l <= r; break;
        default:
            LOG_ERR("Unrecognized comparison op.");
            return NULL;
    }

    return bool_node(final, p->line, c);
}

Node *rel_float(Node *p, Node *a, Node *b, Context *c) {
    float l = a->flo.value;
    float r = b->flo.value;
    bool final;

    switch (p->rel.op) {
        case R_EQ:  final = l == r; break;
        case R_NEQ: final = l != r; break;
        case R_GE:  final = l > r; break;
        case R_GEQ: final = l >= r; break;
        case R_LE:  final = l < r; break;
        case R_LEQ: final = l <= r; break;
        default:
            LOG_ERR("Unrecognized comparison op.");
            return NULL;
    }

    return bool_node(final, p->line, c);
}

Node *rel_bool(Node *p, Node *a, Node *b, Context *c) {
    int l = a->boo.value ? 1 : 0;
    int r = b->boo.value ? 1 : 0;
    bool final;

    switch (p->rel.op) {
        case R_EQ:  final = l == r; break;
        case R_NEQ: final = l != r; break;
        case R_GE:  final = l > r; break;
        case R_GEQ: final = l >= r; break;
        case R_LE:  final = l < r; break;
        case R_LEQ: final = l <= r; break;
        default:
            LOG_ERR("Unrecognized comparison op.");
            return NULL;
    }

    return bool_node(final, p->line, c);
}

/* Perform type coercion on value nodes.
 * If input node is a constant, reset it to a
 * constant of the target type. If not,
 * add a new CAST unary on top of the existing node.
 * Returns a value node. */
Node *coerce(Node *node, Type *type, Context *c) {
    if (!node->type) {
        EMIT_SEMANTIC_ERR(
            node->line, "Expecting an expression."
        );
        c->has_semantic_error = true;
        return NULL;
    }

    if (node->type == type) return node;

    /* Handle constants */
    if (node->node_id == N_NUM) {
        int val = node->num.value;
        switch (type->id) {
            case T_FLOAT:
                node->node_id = N_FLO;
                node->type = type;
                node->flo.value = (float) val;
                return node;
            case T_BOOL:
                node->node_id = N_BOOL;
                node->type = type;
                node->boo.value = val != 0;
                return node;
            default:
                EMIT_SEMANTIC_ERR(
                    node->line, "Type coersion failed. Type IDs: %d, %d.",
                    node->type->id, type->id
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }
    else if (node->node_id == N_FLO) {
        float val = node->flo.value;
        switch (type->id) {
            case T_INT:
                node->node_id = N_NUM;
                node->type = type;
                node->num.value = (int) val;
                return node;
            case T_BOOL:
                node->node_id = N_BOOL;
                node->type = type;
                node->boo.value = val != 0;
                return node;
            default:
                EMIT_SEMANTIC_ERR(
                    node->line, "Type coersion failed. Type IDs: %d, %d.",
                    node->type->id, type->id
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }
    else if (node->node_id == N_BOOL) {
        bool val = node->boo.value;
        switch (type->id) {
            case T_INT:
                node->node_id = N_NUM;
                node->type = type;
                node->num.value = (int) val;
                return node;
            case T_FLOAT:
                node->node_id = N_FLO;
                node->type = type;
                node->boo.value = (float) val;
                return node;
            default:
                EMIT_SEMANTIC_ERR(
                    node->line, "Type coersion failed. Type IDs: %d, %d.",
                    node->type->id, type->id
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }

    /* Handle expressions with variables */
    else {
        Node *output;
        switch (type->id) {
            case T_INT:
                output = unary_node(U_CAST_INT, node, node->line, c);
                output->type = type;
                return output;
            case T_FLOAT:
                output = unary_node(U_CAST_FLOAT, node, node->line, c);
                output->type = type;
                return output;
            case T_BOOL:
                output = unary_node(U_CAST_BOOL, node, node->line, c);
                output->type = type;
                return output;
            default:
                EMIT_SEMANTIC_ERR(
                    node->line, "Type coersion failed. Type IDs: %d, %d.",
                    node->type->id, type->id
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }
}

/* Convert a value node to a node representing
 * conditional flow.
 * If a node is a constant, convert the
 * the node to boolean-> Else, add a NOT_ZERO unary
 * on top of the existing node. */
Node *to_cond(Node *node, Context *c) {
    if (!node->type) {
        EMIT_SEMANTIC_ERR(
            node->line,
            "Expecting an expression; got a conditional."
        );
        c->has_semantic_error = true;
        return NULL;
    }

    /* Handle constants */
    if (node->node_id == N_NUM) {
        int val = node->num.value;
        node->node_id = N_BOOL;
        node->type = NULL;
        node->boo.value = val != 0;
        return node;
    }
    else if (node->node_id == N_FLO) {
        float val = node->flo.value;
        node->node_id = N_BOOL;
        node->type = NULL;
        node->boo.value = val != 0;
        return node;
    }
    else if (node->node_id == N_BOOL) {
        node->type = NULL;
        return node;
    }

    /* Handle expressions with variables */
    return unary_node(U_NOT_ZERO, node, node->line, c);
}

/* Analyzers */

/* Orchestrator for semantic analysis of value nodes.
 * Input node must represent an expression-> Output node
 * must have an assigned type. */
Node *expr_analyzer(Node *node, size_t curr, Context *c) {
    Node *output = node;
    switch (node->node_id) {
        case N_ARITH:
            output = arith_analyzer(node, curr, c);
            if (!output) return NULL;
            break;

        case N_LOGIC:
            output = logic_analyzer(node, curr, c);
            if (!output) return NULL;
            output->type = c->bool_type;
            break;

        case N_REL:
            output = rel_analyzer(node, curr, c);
            if (!output) return NULL;
            output->type = c->bool_type;
            break;

        case N_LOC:
            output = loc_analyzer(node, curr, c);
            if (!output) return NULL;
            break;

        case N_UNARY:
            if (node->unary.op == U_NOT) {
                output = not_analyzer(node, curr, c);
                if (!output) return NULL;
                output->type = c->bool_type;
            }
            else if (node->unary.op == U_NEG) {
                output = neg_analyzer(node, curr, c);
                if (!output) return NULL;
            }
            else {
                EMIT_SEMANTIC_ERR(
                    node->line,
                    "Invalid unary operator. ID: %d.", node->unary.op
                );
                c->has_semantic_error = true;
                return NULL;
            }
            break;

        case N_NUM:     break;
        case N_FLO:     break;
        case N_BOOL:    break;

        case N_ID:
            output = id_analyzer(node, curr, c);
            if (!output) return NULL;
            break;

        default:
            EMIT_SEMANTIC_ERR(
                node->line, "Expecting an expression."
            );
            c->has_semantic_error = true;
            return NULL;
    }

    /* Validates output node has type assigned */
    if (!output->type) {
        EMIT_SEMANTIC_ERR(
            node->line,
            "Expecting an expression; got a conditional."
        );
        return NULL;
    }

    return output;
}

/* Orchestrator for semantic analysis of conditional
 * statements. Output must be a conditional node
 * where type is set to NULL. */
Node *cond_analyzer(Node *node, size_t curr, Context *c) {
    Node *output = node;
    switch (node->node_id) {
        case N_LOGIC:
            output = logic_analyzer(node, curr, c);
            if (!output) return NULL;
            break;

        case N_REL:
            output = rel_analyzer(node, curr, c);
            if (!output) return NULL;
            break;

        case N_UNARY:
            if (node->unary.op == U_NOT) {
                output = not_analyzer(node, curr, c);
                if (!output) return NULL;
            }
            else if (node->unary.op == U_NEG) {
                /* Run cond_analyzer on the child
                 * because '-' does not affect the outcome
                 * of a conditional. */
                output = cond_analyzer(node->unary.child, curr, c);
                if (!output) return NULL;
            }
            else {
                EMIT_SEMANTIC_ERR(
                    node->line,
                    "Invalid unary operator. ID: %d.", node->unary.op
                );
                c->has_semantic_error = true;
                return NULL;
            }
            break;

        case N_ARITH:
            output = to_cond(arith_analyzer(node, curr, c), c);
            if (!output) return NULL;
            break;

        case N_LOC:
            output = to_cond(loc_analyzer(node, curr, c), c);
            if (!output) return NULL;
            break;

        case N_ID:
            output = to_cond(id_analyzer(node, curr, c), c);
            if (!output) return NULL;
            break;

        case N_NUM:
            output = to_cond(node, c);
            if (!output) return NULL;
            break;

        case N_FLO:
            output = to_cond(node, c);
            if (!output) return NULL;
            break;

        case N_BOOL:
            output = to_cond(node, c);
            if (!output) return NULL;
            break;

        default:
            EMIT_SEMANTIC_ERR(
                node->line, "Expecting a conditional statement."
            );
            c->has_semantic_error = true;
            return NULL;
    }

    /* Validates output is a conditional node. */
    if (output->type) {
        EMIT_SEMANTIC_ERR(
            node->line,
            "Expecting a conditional; got an expression."
        );
        return NULL;
    }

    return output;
}

/* Orchestrator for semantic analysis of statement nodes.
 * Declarations are excluded. */
ErrCode stmt_analyzer(Node *node, size_t curr, Context *c) {
    ErrCode e;
    switch (node->node_id) {
        case N_IF:
            node->s_if.cond = cond_analyzer(node->s_if.cond, curr, c);
            if (!node->s_if.cond) return ABORTED;

            e = stmt_analyzer(node->s_if.stmt, curr, c);
            if (e) return e;

            if (node->s_if.estmt) {
                return stmt_analyzer(node->s_if.estmt, curr, c);
            }
            return NO_ERR;

        case N_WHILE:
            node->s_while.cond = cond_analyzer(node->s_while.cond, curr, c);
            if (!node->s_while.cond) return ABORTED;
            return stmt_analyzer(node->s_while.stmt, curr, c);

        case N_DO:
            node->s_do.cond = cond_analyzer(node->s_do.cond, curr, c);
            if (!node->s_do.cond) return ABORTED;
            return stmt_analyzer(node->s_do.stmt, curr, c);

        case N_ASSIGN:
            return assign_analyzer(node, curr, c);

        case N_BLOCK:
            return block_analyzer(node, curr, c);

        case N_BREAK: return NO_ERR;

        default:
            EMIT_SEMANTIC_ERR(
                node->line, "Expecting a statement."
            );
            c->has_semantic_error = true;
            return BUG_DETECTED;
    }
}

/* Checks a variable is declared in prev scope.
 * Returns a value node. */
Node *id_analyzer(Node *node, size_t curr, Context *c) {
    char *name = node->id.name;

    /* Ensure the symbol is declared in some prev scope */
    Symbol *s = get_sym(name, curr, c);
    if (!s) {
        EMIT_SEMANTIC_ERR(
            node->line, "Variable has not been declared."
        );
    }
    node->id.symbol = s;
    node->type = s->type;
    return node;
}

/* Returns a value node. */
Node *arith_analyzer(Node *node, size_t curr, Context *c) {
    Node *left = node->arith.left;
    Node *right = node->arith.right;

    left = expr_analyzer(left, curr, c);
    if (!left) return NULL;
    right = expr_analyzer(right, curr, c);
    if (!right) return NULL;

    node->type = max_type(left->type, right->type);
    left = node->arith.left = coerce(left, node->type, c);
    if (!left) return NULL;
    right = node->arith.right = coerce(right, node->type, c);
    if (!right) return NULL;

    /* If both children are constants, fold the parent
     * node to a constant. */
    if (is_const(left) && is_const(right)) {
        switch(left->type->id) {
            case T_INT:     return arith_int(node, left, right, c);
            case T_FLOAT:   return arith_float(node, left, right, c);
            case T_BOOL:    return arith_bool(node, left, right, c);
            default:
                EMIT_SEMANTIC_ERR(
                    node->line,
                    "Invalid constant type for arithmetic operations."
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }
    return node;
}

/* Returns a value node */
Node *loc_analyzer(Node *node, size_t curr, Context *c) {
    /* If node is a leaf ID node, check if it exists
     * in the symbol table */
    if (node->node_id == N_ID) 
        return id_analyzer(node, curr, c);

    else if (node->node_id == N_LOC) {
        Node *left = node->loc.left;
        left = loc_analyzer(left, curr, c);
        if (!left) return NULL;

        /* Assign type to be the element of left child's type */
        if (left->type->id != T_ARRAY) {
            EMIT_SEMANTIC_ERR(
                node->line, "Cannot access beyond declared array dimension."
            );
            c->has_semantic_error = true;
            return NULL;
        }
        node->type = left->type->elem;

        /* Cast index into an int */
        Node *ind = node->loc.ind;
        ind = expr_analyzer(ind, curr, c);
        if (!ind) return NULL;
        ind = coerce(ind, c->int_type, c);
        if (!ind) return NULL;

        node->loc.left = left;
        node->loc.ind = ind;
        return node;
    }

    return NULL;
}

/* Returns a value node. */
Node *neg_analyzer(Node *node, size_t curr, Context *c) {
    /* Run expr_analyzer on the child node. */
    Node *child = node->unary.child;
    child = expr_analyzer(child, curr, c);
    if (!child) return NULL;

    /* If the child is a constant, fold the parent node. */
    if (child->node_id == N_FLO)
        return float_node(-child->flo.value, node->line, c);
    if (child->node_id == N_NUM)
        return num_node(-child->num.value, node->line, c);
    if (child->node_id == N_BOOL)
        return num_node(child->boo.value ? -1 : 0, node->line, c);

    /* If the child has bool type, cast to integer */
    if (child->type->id == T_BOOL) {
        node->unary.child = coerce(child, c->int_type, c);
        if (!node->unary.child) return NULL;
        node->type = c->int_type;
    }
    /* Otherwise, let parent inherit child's type */
    else if (
        child->type->id == T_INT ||
        child->type->id == T_FLOAT
    ) node->type = child->type;

    else {
        EMIT_SEMANTIC_ERR(
            node->line, "Invalid expression following `-`."
        );
        c->has_semantic_error = true;
        return NULL;
    }

    return node;
}

/* Returns a conditional node. */
Node *not_analyzer(Node *node, size_t curr, Context *c) {
    /* If the child node represents a value, run expr_analyzer
     * and cast the child to boolean-> */
    Node *child = node->unary.child;
    if (is_not_cond(child)) {
        child = expr_analyzer(child, curr, c);
        if (!child) return NULL;
        child = node->unary.child = coerce(child, c->bool_type, c);
        if (!child) return NULL;
    }
    /* Else, run cond_analyzer on child. */
    else if (is_cond(child)) {
        child = node->unary.child = cond_analyzer(child, curr, c);
        if (!child) return NULL;
    }

    else {
        EMIT_SEMANTIC_ERR(
            node->line, "Invalid expression following `!`."
        );
        c->has_semantic_error = true;
        return NULL;
    }

    /* If child is a constant, fold the parent
     * node to a bool node. Note that the child
     * can only be a bool node. */
    if (child->node_id == N_BOOL) {
        node->node_id = N_BOOL;
        node->type = NULL;
        node->boo.value = !child->boo.value;
    }
    return node;
}

/* Returns a conditional node. */
Node *rel_analyzer(Node *node, size_t curr, Context *c) {
    /* Run expr_analyzer on both children. */
    Node *left = expr_analyzer(node->rel.left, curr, c);
    if (!left) return NULL;
    Node *right = expr_analyzer(node->rel.right, curr, c);
    if (!right) return NULL;

    /* Cast children's types to max(left.type, right.type). */
    Type *m = max_type(left->type, right->type);
    node->rel.left = coerce(left, m, c);
    if (!node->rel.left) return NULL;
    node->rel.right = coerce(right, m, c);
    if (!node->rel.right) return NULL;

    /* If both children are constants, fold the parent
     * node to a constant. */
    if (is_const(left) && is_const(right)) {
        switch(left->type->id) {
            case T_INT:     return rel_int(node, left, right, c);
            case T_FLOAT:   return rel_float(node, left, right, c);
            case T_BOOL:    return rel_bool(node, left, right, c); 
            default:
                EMIT_SEMANTIC_ERR(
                    node->line,
                    "Invalid constant type for comparison operations."
                );
                c->has_semantic_error = true;
                return NULL;
        }
    }
    return node;
}

/* Returns a conditional node. */
Node *logic_analyzer(Node *node, size_t curr, Context *c) {
    /* If either child node represents a value,
     * run expr_analyzer on the child and cast the node
     * to a conditional. Otherwise, run cond_analyzer. */
    Node *left = node->logic.left;
    if (is_not_cond(left)) {
        left = expr_analyzer(left, curr, c);
        if (!left) return NULL;
        left = node->logic.left = coerce(left, c->bool_type, c);
        if (!left) return NULL;
    }
    else {
        left = node->logic.left = cond_analyzer(left, curr, c);
        if (!left) return NULL;
    }

    Node *right = node->logic.right;
    if (is_not_cond(right)) {
        right = expr_analyzer(right, curr, c);
        if (!right) return NULL;
        right = node->logic.right = coerce(right, c->bool_type, c);
        if (!right) return NULL;
    }
    else {
        right = node->logic.right = cond_analyzer(right, curr, c);
        if (!right) return NULL;
    }

    /* If both children are constants, fold the top node
     * to a constant. Note that a constant must be a boolean
     * due to prior logic. */
    if (left->node_id == N_BOOL && right->node_id == N_BOOL) {
        bool val = node->logic.op == L_AND ?
            left->boo.value && right->boo.value :
            left->boo.value || right->boo.value;
        node->node_id = N_BOOL;
        node->type = NULL;
        node->boo.value = val;
    }
    return node;
}

ErrCode assign_analyzer(Node *node, size_t curr, Context *c) {
    Node *lvalue = node->assign.lvalue;
    Node *rvalue = node->assign.rvalue;

    /* Identifies correct type of LHS */
    node->assign.lvalue = loc_analyzer(lvalue, curr, c);
    if (!node->assign.lvalue) return ABORTED;

    /* Identifies correct type of RHS */
    rvalue = expr_analyzer(rvalue, curr, c);
    if (!rvalue) return ABORTED;

    /* Check if types match, and coerce if applicable */
    node->assign.rvalue = coerce(rvalue, lvalue->type, c);
    if (!node->assign.rvalue) return ABORTED;

    return NO_ERR;
}

ErrCode decl_analyzer(Node *node, size_t curr, Context *c) {
    Node *id = node->decl.id;

    /* Check if the ID already exists in current scope */
    if (get_sym(id->id.name, curr, c)) {
        EMIT_SEMANTIC_ERR(node->line, "Variable already exists in scope.");
    }

    /* Create new symbol and push to symbol table */
    Symbol *s = newsym(id->id.name, node->decl.type, c);
    if (!s) return INTERNAL_ERR;

    ErrCode e = put_sym(s, curr, c);
    if (e) {
        LOG_ERR("Failed to put symbol in symbol table. Error code: %d.", e);
        return e;
    }

    /* Set id node's symbol attribute */
    id->id.symbol = s;

    return NO_ERR;
}

ErrCode block_analyzer(Node *node, size_t prev_st, Context *c) {
    /* Create new scope */
    ErrCode e = sym_table(prev_st, c);
    if (e) {
        LOG_ERR("Failed to create new symbol table scope.");
        return e;
    }
    size_t curr = c->st_array->size - 1;

    Array *decls = node->block.decls;
    for (int d = 0; d < decls->size; d++) {
        e = decl_analyzer(((Node **) decls->arr)[d], curr, c);
        if (e) return e;
    }

    Array *stmts = node->block.stmts;
    for (int s = 0; s < stmts->size; s++) {
        e = stmt_analyzer(((Node **) stmts->arr)[s], curr, c);
        if (e) return e;
    }

    return NO_ERR;
}

ErrCode program_analyzer(Node *root, Context *c) {
    if (!root) {
        EMIT_SEMANTIC_ERR(
            0,
            "Unrecoverable error in parsing. "
            "Semantic analysis aborted."
        );
        return ABORTED;
    }
    return block_analyzer(root, SIZE_MAX, c);
}
