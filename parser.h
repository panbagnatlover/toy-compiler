/* Parser: builds AST tree

   Rewrite grammar to LL(1):

    program     -> block
    block       -> { decls stmts }

    decls       -> decl decls | e
    decl        -> type id ;
    type        -> basic array
    array       -> [ num ] array | e

    stmts       -> stmt stmts | e
    stmt        -> loc = expr ;
                 | if ( expr ) stmt estmt
                 | while ( expr ) stmt
                 | do stmt while ( expr ) ;
                 | break ;
                 | block
    estmt       -> else stmt | e
    loc         -> id access
    access      -> [ expr ] access | e

    expr        -> join joins
    joins       -> || join joins | e
    join        -> equality equalities
    equalities  -> && equality equalities | e
    equality    -> rel rels
    rels        -> == rel rels | != rel rels | e
    rel         -> arith ariths
    ariths      -> < arith | <= arith | > arith
                 | >= arith | e
    arith       -> term terms
    terms       -> + term terms | - term terms | e
    term        -> unary unaries
    unaries     -> * unary unaries | / unary unaries | e
    unary       -> ! unary | - unary | factor
    factor      -> ( expr ) | loc | num | real | true | false

 * Error handling pattern:
 * Every occurrence of an error would mark
 * has_parsing_error true. If an error is recoverable,
 * parser would either find the next valid token
 * or add a missing token and continue parsing. If
 * an error is unrecoverable, a NULL node is returned.
 * An unrecoverable error would trigger all parent
 * node to return NULL. Semantic analysis would
 * continue in the case of a recoverable error, but
 * the program would stop before translation.
 */

#ifndef H_PARSER
#define H_PARSER

#include "node.h"
#include "context.h"

Node *factor_parser(char *input, Context *c);
Node *unary_parser(char *input, Context *c);
Node *term_parser(char *input, Context *c);
Node *arith_parser(char *input, Context *c);
Node *rel_parser(char *input, Context *c);
Node *equality_parser(char *input, Context *c);
Node *join_parser(char *input, Context *c);
Node *expr_parser(char *input, Context *c);
Node *if_parser(char *input, Context *c);
Node *while_parser(char *input, Context *c);
Node *do_parser(char *input, Context *c);
Node *loc_parser(char *input, Context *c);
Node *assign_parser(char *input, Context *c);
Node *stmt_parser(char *input, Context *c);
ErrCode stmts_parser(char *input, Node *block, Context *c);
Type *array_parser(char *input, int ind, Type *basic, Context *c);
Type *type_parser(char *input, Context *c);
Node *decl_parser(char *input, Context *c);
ErrCode decls_parser(char *input, Node *block, Context *c);
Node *block_parser(char *input, Context *c);
Node *program_parser(char *input, Context *c);

#endif
