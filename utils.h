/* Util functions */

#ifndef H_UTILS
#define H_UTILS

#include "type.h"
#include "dict.h"
#include "node.h"
#include "symbol.h"
#include "label.h"
#include "operand.h"
#include "context.h"
#include "error.h"

/* APIs */

/* String */
Token get_str_token(char *word, size_t len, Context *c);
ErrCode seed_keywords(Context *c);
size_t str_hash(const void *key, const size_t cap);
bool str_comp(const void *a, const void *b);

/* Types */
Type *max_type(Type *a, Type *b);
ErrCode seed_basic_type(Context *c);
Type *getarray(Type *elem, int width, Context *c);
size_t type_hash(const void *key, const size_t cap);
bool type_comp(const void *a, const void *b);

/* Nodes */
void free_node_block(ArenaBlock *block);
Node *block_node(int line, Context *c);
Node *decl_node(Type *type, Node *id, int line, Context *c);
Node *assign_node(Node *lvalue, Node *rvalue, int line, Context *c);
Node *if_node(Node *cond, Node *stmt, Node *estmt, int line, Context *c);
Node *while_node(Node *cond, Node *stmt, int line, Context *c);
Node *do_node(Node *cond, Node *stmt, int line, Context *c);
Node *break_node(int line, Context *c);
Node *arith_node(Op op, Node *left, Node *right, int line, Context *c);
Node *float_node(float value, int line, Context *c);
Node *num_node(int value, int line, Context *c);
Node *bool_node(bool value, int line, Context *c);
Node *id_node(char *name, int line, Context *c);
Node *loc_node(Node *left, Node *ind, Node *base, int line, Context *c);
Node *logic_node(Logic op, Node *left, Node *right, int line, Context *c);
Node *rel_node(Rel op, Node *left, Node *right, int line, Context *c);
Node *unary_node(Unary op, Node *child, int line, Context *c);

/* Symbols */
ErrCode put_sym(Symbol *sym, size_t ind, Context *c);
Symbol *get_sym(char *name, size_t ind, Context *c);
Symbol *newsym(char *name, Type *type, Context *c);
Symbol *newtemp(Type *type, Context *c);
ErrCode sym_table(size_t parent_ind, Context *c);
size_t str_intern_hash(const void *key, const size_t cap);
bool str_intern_comp(const void *a, const void *b);
void free_sym_table_array_obj(void *item);

/* Labels */
Label *newlabel(Context *c);
void evalabel(Label *l, Context *c);

/* Operands */
Operand bool_operand(bool val);
Operand num_operand(int val);
Operand float_operand(float val);
Operand var_operand(Symbol *var);
Operand temp_operand(Symbol *t);
Operand err_operand(void);

#endif
