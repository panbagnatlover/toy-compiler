#include <stdio.h>
#include "array.h"
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "utils.h"
#include "log.h"

/* Greedily match the next valid token */
Token next_valid_token(char *input, TokenID token_id, Context *c) {
    Token token = scan(input, c); match(c);
    if (token.token_id == token_id) return token;

    EMIT_SYNTAX_ERR(
        c->t_line,
        "Invalid token id %d when expecting %d.",
        token.token_id,
        token_id
    );
    c->has_parsing_error = true;

    do {
        token = scan(input, c);
        match(c);
    }
    while (token.token_id != token_id && token.token_id != EOF_TOKEN);

    return token;
}

/* Production:
 *  factor -> ( expr ) | loc | num | real | true | false
 * Only called by unary_parser. */
Node *factor_parser(char *input, Context *c) {
    Token token = scan(input, c);

    switch (token.token_id) {
        case LPR:
            match(c);

            /* Parse expression */
            Node *expr = expr_parser(input, c);
            if (!expr) return NULL;

            /* Match ')' */
            token = scan(input, c);
            if (token.token_id == RPR) match(c);
            else {
                EMIT_SYNTAX_ERR(
                    c->t_line, "Missing closed ')'."
                );
                c->has_parsing_error = true;
            }
            return expr;

        case FLO:
            match(c);
            return float_node(token.flo_val, c->t_line, c);

        case NUM:
            match(c);
            return num_node(token.num_val, c->t_line, c);

        case ID:
            return loc_parser(input, c);

        case TRU:
            match(c);
            return bool_node(true, c->t_line, c);

        case FAL:
            match(c);
            return bool_node(false, c->t_line, c);

        default:
            EMIT_SYNTAX_ERR(
                c->t_line, "Invalid token type."
            );
            c->has_parsing_error = true;
            return NULL;
    }
}

/* Production:
 *  unary -> ! unary | - unary | factor */
Node *unary_parser(char *input, Context *c) {
    Token token = scan(input, c);

    if (token.token_id == MIN || token.token_id == NOT) {
        int line = c->t_line;
        match(c);

        Node *child = unary_parser(input, c);
        if (!child) return NULL;
        Node *unary = unary_node(
            token.token_id == MIN ? U_NEG : U_NOT,
            child, line, c
        );
        if (!unary) return NULL;

        return unary;
    }
    return factor_parser(input, c);
}

/* Productions:
 *  term -> unary unaries
 *  unaries -> * unary unaries | / unary unaries | e */
Node *term_parser(char *input, Context *c) {
    Node *left = unary_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion */
    while (token.token_id == MUL || token.token_id == DIV) {
        int line = c->t_line;
        match(c);

        Node *right = unary_parser(input, c);
        if (!right) return NULL;
        Node *arith = arith_node(
            token.token_id == MUL ? O_MUL : O_DIV,
            left, right, line, c
        );
        if (!arith) return NULL;

        left = arith;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 *  arith -> term terms
 *  terms -> + term terms | - term terms | e */
Node *arith_parser(char *input, Context *c) {
    Node *left = term_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion */
    while (token.token_id == PLU || token.token_id == MIN) {
        int line = c->t_line;
        match(c);

        Node *right = term_parser(input, c);
        if (!right) return NULL;
        Node *arith = arith_node(
            token.token_id == PLU ? O_PLU : O_MIN,
            left, right, line, c
        );
        if (!arith) return NULL;

        left = arith;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 * rel -> arith ariths
 * ariths -> < arith | <= arith | > arith
        | >= arith | e */
Node *rel_parser(char *input, Context *c) {
    Node *left = arith_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion. */
    while (
        token.token_id == LE ||
        token.token_id == LEQ ||
        token.token_id == GE ||
        token.token_id == GEQ
    ) {
        int line = c->t_line;
        match(c);

        Node *right = arith_parser(input, c);
        if (!right) return NULL;

        /* Find the right op code */
        Rel op;
        switch (token.token_id) {
            case LE:
                op = R_LE;
                break;
            case LEQ:
                op = R_LEQ;
                break;
            case GE:
                op = R_GE;
                break;
            case GEQ:
                op = R_GEQ;
                break;
            default:
                LOG_ERR("Invalid token rel op.");
                return NULL;
        }
        Node *logic = rel_node(op, left, right, line, c);
        if (!logic) return NULL;

        left = logic;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 * equality -> rel rels
 * rels -> == rel rels | != rel rels | e */
Node *equality_parser(char *input, Context *c) {
    Node *left = rel_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion. */
    while (token.token_id == EQL || token.token_id == NEQ) {
        int line = c->t_line;
        match(c);

        Node *right = rel_parser(input, c);
        if (!right) return NULL;
        Node *logic = rel_node(
            token.token_id == EQL ? R_EQ : R_NEQ,
            left, right, line, c
        );
        if (!logic) return NULL;

        left = logic;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 *  join -> equality equalities
 *  equalities -> && equality equalities | e */
Node *join_parser(char *input, Context *c) {
    Node *left = equality_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion. */
    while (token.token_id == AND) {
        int line = c->t_line;
        match(c);

        Node *right = equality_parser(input, c);
        if (!right) return NULL;
        Node *logic = logic_node(L_AND, left, right, line, c);
        if (!logic) {
            LOG_ERR("Failed to create logic node.");
            return NULL;
        }

        left = logic;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 *  expr -> join joins
 *  joins -> || join joins | e */
Node *expr_parser(char *input, Context *c) {
    Node *left = join_parser(input, c);
    if (!left) return NULL;
    Token token = scan(input, c);

    /* Removing tail recursion. */
    while (token.token_id == OR) {
        int line = c->t_line;
        match(c);

        Node *right = join_parser(input, c);
        if (!right) return NULL;

        Node *logic = logic_node(L_OR, left, right, line, c);
        if (!logic) return NULL;

        left = logic;
        token = scan(input, c);
    }
    return left;
}

/* Productions:
 *  if ( expr ) stmt estmt
 *  estmt -> else stmt | e
 * 
 * Only called by stmt parser.
 * Note that the grammar is ambiguous. Observe
 * the following: if (B) if (B1) S1 else S2.
 * We impose that the else-statement belong to
 * the closest if-clause.
 */
Node *if_parser(char *input, Context *c) {
    int line = c->t_line;

    /* Match '(' */
    Token token = scan(input, c);
    if (token.token_id == LPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Expecting '(' after `if`."
        );
        c->has_parsing_error = true;
    }

    Node *cond = NULL;
    Node *stmt = NULL;
    Node *estmt = NULL;

    /* Match expression */
    cond = expr_parser(input, c);
    if (!cond) return NULL;

    /* Match ')' */
    token = scan(input, c);
    if (token.token_id == RPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Missing ')'."
        );
        c->has_parsing_error = true;
    }

    /* Match statement */
    stmt = stmt_parser(input, c);
    if (!stmt) {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Invalid statement after if-condition."
        );
        c->has_parsing_error = true;
        return NULL;
    }

    /* Match else statement */
    token = scan(input, c);
    if (token.token_id == ELSE) {
        match(c);

        estmt = stmt_parser(input, c);
        if (!estmt) {
            EMIT_SYNTAX_ERR(
                c->t_line,
                "Invalid statement after `else`."
            );
            c->has_parsing_error = true;
            return NULL;
        }
    }

    /* Create new node */
    return if_node(cond, stmt, estmt, line, c);
}

/* Production:
 *  stmt -> while ( expr ) stmt
 * Only called by stmt parser. */
Node *while_parser(char *input, Context *c) {
    int line = c->t_line;

    /* Match '(' */
    Token token = scan(input, c);
    if (token.token_id == LPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Expecting '(' after 'while'."
        );
        c->has_parsing_error = true;
    }
    match(c);

    /* Match expression */
    Node *cond = expr_parser(input, c);
    if (!cond) return NULL;

    /* Match ')' */
    token = scan(input, c);
    if (token.token_id == RPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Missing ')'."
        );
        c->has_parsing_error = true;
    }

    /* Match stmt */
    Node *stmt = stmt_parser(input, c);
    if (!stmt) {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Invalid statement after while-condition."
        );
        c->has_parsing_error = true;
        return NULL;
    }

    /* Create new node */
    return while_node(cond, stmt, line, c);
}

/* Only called by stmt parser */
Node *do_parser(char *input, Context *c) {
    int line = c->t_line;

    /* Match stmt */
    Node *stmt = stmt_parser(input, c);
    if (!stmt) {
        EMIT_SYNTAX_ERR(
            c->t_line, "Invalid statement after `do`."
        );
        c->has_parsing_error = true;
        return NULL;
    }

    /* Match WHILE */
    Token token = next_valid_token(input, WHILE, c);
    if (token.token_id == EOF_TOKEN) return NULL;

    /* Match '(' */
    token = scan(input, c);
    if (token.token_id == LPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Expecting '(' after 'while'."
        );
        c->has_parsing_error = true;
    }

    /* Match expr */
    Node *cond = expr_parser(input, c);
    if (!cond) return NULL;

    /* Match ')' */
    token = scan(input, c);
    if (token.token_id == RPR) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line, "Missing ')'."
        );
        c->has_parsing_error = true;
    }

    return do_node(cond, stmt, line, c);
}

Node *break_parser(char *input, Context *c) {
    Token token = scan(input, c);

    if (token.token_id == SEM) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Missing ';' after `break`."
        );
        c->has_parsing_error = true;
    }

    return break_node(c->t_line, c);
}

/* Productions:
 * loc -> id access  
 * access -> [ bool ] access | e
 * 
 * Implementation removes tail recursion.
 * Called by assign parser or factor parser.
 * Sample tree for ID[expr][expr][expr]:
 *         LOC
 *        /   \
 *      LOC   [expr]
 *     /   \
 *    LOC  [expr]
 *   /  \
 *  ID  [expr]
 * Due to right recursion in the grammar, need
 * to pass node.base as inherited attribute so
 * the top level node can retrieve the base element
 * during translation.
 */
Node *loc_parser(char *input, Context *c) {
    /* Match ID */
    Token token = scan(input, c);
    match(c);

    Node *left = id_node(token.word, c->t_line, c);
    if (!left) return NULL;

    token = scan(input, c);

    /* Match array access */
    while (token.token_id == LSQ) {
        match(c);
        int line = c->t_line;

        Node *ind = expr_parser(input, c);
        if (!ind) return NULL;
        Node *base = left->node_id == N_LOC ? left->loc.base : left;

        /* Create a new LOC node */
        Node *loc = loc_node(left, ind, base, line, c);
        if (!loc) return NULL;

        /* Match ']' */
        token = scan(input, c);
        if (token.token_id == RSQ) match(c);
        else {
            EMIT_SYNTAX_ERR(
                c->t_line, "Missing closed ']'."
            );
            c->has_parsing_error = true;
        }

        left = loc;
        token = scan(input, c);
    }
    return left;
}

/* Production:
 * stmt -> loc = expr ;
 * Only called by stmt parser.
 * The LHS node can be either ID or LOC. */
Node *assign_parser(char *input, Context *c) {
    /* Match LHS */
    Node *lvalue = loc_parser(input, c);
    if (!lvalue) return NULL;

    /* Match '=' */
    Token token = next_valid_token(input, ASG, c);
    if (token.token_id == EOF_TOKEN) return NULL;
    int line = c->t_line;

    /* Match RHS */
    Node *rvalue = expr_parser(input, c);
    if (!rvalue) return NULL;

    /* Match ';' */
    token = scan(input, c);
    if (token.token_id == SEM) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Missing ';' after declaration."
        );
        c->has_parsing_error = true;
    }

    /* Create ASSIGN node */
    return assign_node(lvalue, rvalue, line, c);
}

/* Stmt orchestrator. Only called by stmts parser */
Node *stmt_parser(char *input, Context *c) {
    Token token = scan(input, c);
    switch (token.token_id) {
        case IF:    match(c); return if_parser(input, c);
        case WHILE: match(c); return while_parser(input, c);
        case DO:    match(c); return do_parser(input, c);
        case ID:    return assign_parser(input, c);
        case LBR:   return block_parser(input, c);
        case BREAK: match(c); return break_parser(input, c);
        default:    return NULL;
    }
}

/* Only called by block parser */
ErrCode stmts_parser(char *input, Node *block, Context *c) {
    Token token = scan(input, c);

    while (1) {
        Node *stmt = stmt_parser(input, c);

        /* Break out of the loop if no more stmts found */
        if (!stmt && c->curr_token.token_id == RBR) break;

        /* If a child statement results in an
         * unrecoverable error, return INTERNAL_ERR */
        else if (!stmt) return INTERNAL_ERR;

        ErrCode err = push_array(&stmt, block->block.stmts);
        if (err) {
            LOG_ERR("Failed to push stmt node to array.");
            return err;
        }
    }
    return NO_ERR;
}

/* Production:
 * array -> [ num ] array | e
 * 
 * Note that this production cannot be parsed the same
 * way as the grammar for loc, where all attributes
 * are inherited. Here, both inherited and synthesized
 * attributes are present, and therefore there is no
 * tail recursion to be removed.
 */
Type *array_parser(char *input, int ind, Type *basic, Context *c) {
    Token token = scan(input, c);

    if (token.token_id == LSQ) {
        match(c);

        /* Match index */
        token = next_valid_token(input, NUM, c);
        if (token.token_id == EOF_TOKEN) return NULL;
        int curr_ind = token.num_val;

        /* Match ']' */
        token = scan(input, c);
        if (token.token_id == RSQ) match(c);
        else {
            EMIT_SYNTAX_ERR(c->t_line, "Missing ']'.");
            c->has_parsing_error = true;
        }

        /* Create array */
        Type *elem = array_parser(input, curr_ind, basic, c);
        if (!elem) return NULL;
        return getarray(elem, ind * elem->width, c);
    }

    /* If there's no additional array index to be parsed,
     * we've reached the lowest layer of the array */
    return getarray(basic, ind, c);
}

/* Production:
 * type -> basic array
 *
 * Returns a pointer to the type arena.
 * Invoked when token is already identified as BASIC. */
Type *type_parser(char *input, Context *c) {
    Token token = scan(input, c);
    match(c);

    /* Match basic type */
    Type *basic = token.type;

    /* Match array declaration, if applicable */
    token = scan(input, c);
    if (token.token_id == LSQ) {
        match(c);

        /* Match index */
        token = next_valid_token(input, NUM, c);
        if (token.token_id == EOF_TOKEN) return NULL;
        int ind = token.num_val;

        /* Match ']' */
        token = scan(input, c);
        if (token.token_id == RSQ) match(c);
        else {
            EMIT_SYNTAX_ERR(c->t_line, "Missing ']'.");
            c->has_parsing_error = true;
        }

        /* Create array */
        return array_parser(input, ind, basic, c);
    }

    /* Return basic type if there's no array declaration */
    return basic;
}

/* Production:
 *  decl -> type id ;
 * Invoked when token is already identified as BASIC. */
Node *decl_parser(char *input, Context *c) {
    /* Parse type */
    Type *type = type_parser(input, c);
    if (!type) return NULL;

    /* Parse id */
    Token token = next_valid_token(input, ID, c);
    if (token.token_id == EOF_TOKEN) return NULL;

    Node *id = id_node(token.word, c->t_line, c);
    if (!id) return NULL;

    Node *decl = decl_node(type, id, c->t_line, c);
    if (!decl) return NULL;

    /* Match ';' */
    token = scan(input, c);
    if (token.token_id == SEM) match(c);
    else {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Missing ';' after declaration."
        );
        c->has_parsing_error = true;
    }

    return decl;
}

/* Only called by block parser */
ErrCode decls_parser(char *input, Node *block, Context *c) {
    Token token = scan(input, c);

    while (token.token_id == BASIC) {
        Node *decl = decl_parser(input, c);
        if (!decl) return INTERNAL_ERR;

        ErrCode err = push_array(&decl, block->block.decls);
        if (err) {
            LOG_ERR(
                "Failed to push decl node to array. Error code: %d.", err
            );
            return err;
        }

        token = scan(input, c);
    }

    return NO_ERR;
}

Node *block_parser(char *input, Context *c) {
    /* Match '{' */
    Token token = scan(input, c);
    if (token.token_id == LBR) match(c);
    else {
        EMIT_SYNTAX_ERR(c->t_line, "Expecting '{'.");
        c->has_parsing_error = true;
    }

    Node *block = block_node(c->t_line, c);
    if (!block) return NULL;

    /* Match content */
    ErrCode e = decls_parser(input, block, c);
    if (e) return NULL;

    e = stmts_parser(input, block, c);
    if (e) return NULL;

    /* Match '}' */
    token = scan(input, c);
    if (token.token_id == RBR) match(c);
    else {
        EMIT_SYNTAX_ERR(c->t_line, "Expecting '}'.");
        c->has_parsing_error = true;
    }

    return block;
}

/* Main parser entry point of the program. */
Node *program_parser(char *input, Context *c) {
    Node *root = block_parser(input, c);

    if (!root) {
        c->has_parsing_error = true;
        return NULL;
    }

    /* Validate that all text is parsed */
    Token end = scan(input, c);
    if (end.token_id != EOF_TOKEN) {
        EMIT_SYNTAX_ERR(
            c->t_line,
            "Parsing finished before EOF."
        );
        c->has_parsing_error = true;
    }

    return root;
}


