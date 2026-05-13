#include <string.h>
#include <stdint.h>
#include "dict.h"
#include "error.h"
#include "type.h"
#include "node.h"
#include "symbol.h"
#include "label.h"
#include "operand.h"
#include "context.h"
#include "utils.h"
#include "log.h"

/* String */

/* Push a string to arena and put a TokenIDLookup object to map */
ErrCode put_str_token_lookup(
    char *word, size_t len, TokenID token_id, Context *c
) {
    ErrCode e;
    /* `word` represents a pointer to the current string
     * buffer. Output ref represents a pointer to the
     * arena location where the string is copied. */
    ArenaRef aref = push_arena(word, len, c->str_arena);
    if (aref.code) {
        LOG_ERR("Failed to add string to arena.");
        return aref.code;
    }

    TokenIDLookup lookup;
    lookup.token_id = token_id;

    /* If word represents a basic type, populate type_id.
     * Else, populate a string. */
    if (strcmp(aref.ref, "int") == 0) {
        lookup.type = c->int_type;
    }
    else if (strcmp(aref.ref, "float") == 0) {
        lookup.type = c->float_type;
    }
    else if (strcmp(aref.ref, "bool") == 0) {
        lookup.type = c->bool_type;
    }
    else lookup.name = aref.ref;

    /* `str_map` key represents a pointer to the 
     * arena ref. */
    e = put_dict(&aref.ref, &lookup, c->str_map);
    return e;
}

/* Receive char * as input and returns a Token.
 * For ID input, try matching with reserved keyword
 * and return reserved keyword if there's a match. */
Token get_str_token(char *word, size_t len, Context *c) {
    Token token;
    ErrCode e;

    /* If the word already exists in string map, return it */
    TokenIDLookup obj;
    e = get_dict(&word, &obj, c->str_map);
    if (!e) {
        token.token_id = obj.token_id;
        if (token.token_id == ID) token.word = obj.name;
        else if (token.token_id == BASIC) token.type = obj.type;
    }
    /* Otherwise, add the word to arena and 
     * create a new entry in word map */
    else if (e == KEY_NOT_FOUND) {
        ArenaRef aref = push_arena(word, len, c->str_arena);
        if (aref.code) {
            LOG_ERR("Failed to add string to arena.");
            token.token_id = ERR_TOKEN;
            return token;
        }
        TokenIDLookup lookup;
        lookup.name = aref.ref;
        lookup.token_id = ID;

        e = put_dict(&aref.ref, &lookup, c->str_map);
        token.word = aref.ref;
        token.token_id = ID;
    }
    else {
        LOG_ERR("Failed to retrieve token from string map.");
        token.token_id = ERR_TOKEN;
    }

    return token;
}

/* Seed Keywords: do, while, if, else, break,
 * true, false, int, float, bool */
ErrCode seed_keywords(Context *c) {
    ErrCode e;

    char *keywords[10] = {
        "do", "while", "if", "else", "break",
        "true", "false", "int", "float", "bool"
    };
    TokenID ids[10] = {
        DO, WHILE, IF, ELSE, BREAK, TRU, FAL,
        BASIC, BASIC, BASIC
    };

    for (int i = 0; i < 10; i++) {
        char *kw = keywords[i];
        e = put_str_token_lookup(
            kw, strlen(kw) + 1, ids[i], c
        );
        if (e) {
            LOG_ERR("Failed to seed keyword `%s`.", kw);
            return e;
        }
    }
    return NO_ERR;
}

/* Hash func for `str_map`. Key represents a pointer
 * to a pointer to a string in the arena. */
size_t str_hash(const void *key, const size_t cap) {
    size_t hash = 5381;
    size_t i = 0;
    char *item = *(char **) key;
    char c = item[0];

    while (c) {
        i++;
        hash = ((hash << 5) + hash) + c;
        c = item[i];
    }

    return hash & (cap - 1);
}

bool str_comp(const void *a, const void *b) {
    return strcmp(*(char **) a, *(char **) b) == 0;
}

/* Type */

/* Identify the max type of two types by width */
Type *max_type(Type *a, Type *b) {
    return a->width >= b->width ? a : b;
}

/* Seed basic types in arena. Note that basic types
 * don't need to be in type map. */
ErrCode seed_basic_type(Context *c) {
    Type t; ArenaRef aref; ErrCode e;

    t.id = T_INT; t.width = 4; t.elem = NULL;
    aref = push_arena(&t, 1, c->type_arena);
    if (aref.code) {
        LOG_DBG("Failed to seed type INT in arena.");
        return aref.code;
    }
    c->int_type = (Type *) aref.ref;

    t.id = T_FLOAT; t.width = 8; t.elem = NULL;
    aref = push_arena(&t, 1, c->type_arena);
    if (aref.code) {
        LOG_DBG("Failed to seed type FLOAT in arena.");
        return aref.code;
    }
    c->float_type = (Type *) aref.ref;

    t.id = T_BOOL; t.width = 1; t.elem = NULL;
    aref = push_arena(&t, 1, c->type_arena);
    if (aref.code) {
        LOG_DBG("Failed to seed type BOOL in arena.");
        return aref.code;
    }
    c->bool_type = (Type *) aref.ref;

    return NO_ERR;
}

Type *getarray(Type *elem, int width, Context *c) {
    ErrCode e;
    /* If the type already exists, return it */
    Type t; t.id = T_ARRAY; t.elem = elem; t.width = width;
    Type *dst;
    e = get_dict(&t, &dst, c->type_map);
    if (!e) return dst;

    /* Add a new type to arena and put the reference in map */
    ArenaRef aref = push_arena(&t, 1, c->type_arena);
    if (aref.code) {
        LOG_ERR("Failed to add new array type to arena.");
        return NULL;
    }
    e = put_dict(&t, &aref.ref, c->type_map);
    return (Type *) aref.ref;
}

size_t type_hash(const void *key, const size_t cap) {
    Type *t = (Type *) key;
    size_t h = 1469598103934665603ULL;

    h ^= (size_t) t->id + 0x9e3779b9;
    h *= 1099511628211ULL;

    h ^= (size_t) t->width + 0x9e3779b9;
    h *= 1099511628211ULL;

    uintptr_t x = (uintptr_t) t->elem;
    x >>= 4;

    h ^= (size_t) x + 0x9e3779b9;
    h *= 1099511628211ULL;

    return h & (cap - 1);
}

bool type_comp(const void *a, const void *b) {
    Type *type_a = (Type *) a;
    Type *type_b = (Type *) b;
    return type_a->id == type_b->id &&
        type_a->width == type_b->width &&
        type_a->elem == type_b->elem;
}

/* Nodes */

Node *block_node(int line, Context *c) {
    Node n;
    n.node_id = N_BLOCK;
    n.type = NULL;
    n.line = line;

    n.block.decls = array(sizeof(Node *), free_array_obj_generic);
    if (!n.block.decls) return NULL;
    n.block.stmts = array(sizeof(Node *), free_array_obj_generic);
    if (!n.block.stmts) return NULL;

    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *decl_node(Type *type, Node *id, int line, Context *c) {
    Node n;
    n.node_id = N_DECL;
    n.type = NULL;
    n.line = line;
    n.decl.type = type;
    n.decl.id = id;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *assign_node(Node *lvalue, Node *rvalue, int line, Context *c) {
    Node n;
    n.node_id = N_ASSIGN;
    n.type = NULL;
    n.line = line;
    n.assign.lvalue = lvalue;
    n.assign.rvalue = rvalue;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *if_node(Node *cond, Node *stmt, Node *estmt, int line, Context *c) {
    Node n;
    n.node_id = N_IF;
    n.type = NULL;
    n.line = line;
    n.s_if.cond = cond;
    n.s_if.stmt = stmt;
    n.s_if.estmt = estmt;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *while_node(Node *cond, Node *stmt, int line, Context *c) {
    Node n;
    n.node_id = N_WHILE;
    n.type = NULL;
    n.line = line;
    n.s_while.cond = cond;
    n.s_while.stmt = stmt;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *do_node(Node *cond, Node *stmt, int line, Context *c) {
    Node n;
    n.node_id = N_DO;
    n.type = NULL;
    n.line = line;
    n.s_do.cond = cond;
    n.s_do.stmt = stmt;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *break_node(int line, Context *c) {
    Node n;
    n.node_id = N_BREAK;
    n.type = NULL;
    n.line = line;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *arith_node(Op op, Node *left, Node *right, int line, Context *c) {
    Node n;
    n.node_id = N_ARITH;
    n.type = NULL;
    n.line = line;
    n.arith.op = op;
    n.arith.left = left;
    n.arith.right = right;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *float_node(float value, int line, Context *c) {
    Node n;
    n.node_id = N_FLO;
    n.type = c->float_type;
    n.line = line;
    n.flo.value = value;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *num_node(int value, int line, Context *c) {
    Node n;
    n.node_id = N_NUM;
    n.type = c->int_type;
    n.line = line;
    n.num.value = value;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *bool_node(bool value, int line, Context *c) {
    Node n;
    n.node_id = N_BOOL;
    n.type = c->bool_type;
    n.line = line;
    n.boo.value = value;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *id_node(char *name, int line, Context *c) {
    Node n;
    n.node_id = N_ID;
    n.type = NULL;
    n.line = line;
    n.id.name = name;
    n.id.symbol = NULL;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *loc_node(Node *left, Node *ind, Node *base, int line, Context *c) {
    Node n;
    n.node_id = N_LOC;
    n.type = NULL;
    n.line = line;
    n.loc.left = left;
    n.loc.ind = ind;
    n.loc.base = base;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *logic_node(Logic op, Node *left, Node *right, int line, Context *c) {
    Node n;
    n.node_id = N_LOGIC;
    n.type = NULL;
    n.line = line;
    n.logic.op = op;
    n.logic.left = left;
    n.logic.right = right;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *rel_node(Rel op, Node *left, Node *right, int line, Context *c) {
    Node n;
    n.node_id = N_REL;
    n.type = NULL;
    n.line = line;
    n.rel.op = op;
    n.rel.left = left;
    n.rel.right = right;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

Node *unary_node(Unary op, Node *child, int line, Context *c) {
    Node n;
    n.node_id = N_UNARY;
    n.type = NULL;
    n.line = line;
    n.unary.op = op;
    n.unary.child = child;
    ArenaRef aref = push_arena(&n, 1, c->node_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to add node to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

/* Function to free each node in an arena block */
void free_node_block(ArenaBlock *block) {
    Node *arr = (Node *) block->arr;
    if (arr) {
        for (int i = 0; i < block->occupied; i++) {
            if (arr[i].node_id == N_BLOCK) {
                free_array(arr[i].block.decls);
                free_array(arr[i].block.stmts);
            }
        }
        free(arr);
    }
    free(block);
}

/* Symbols & symbol tables */

/* Put symbol to the symbol table located at index ind */
ErrCode put_sym(Symbol *sym, size_t ind, Context *c) {
    SymTable st = ((SymTable *) c->st_array->arr)[ind];
    return put_dict(&sym->name, &sym, st.dict);
}

/* Get a symbol from symbol table by traversing
 * parent scopes. Returns Symbol if there's a declared
 * ID in any of the parent scopes, else NULL */
Symbol *get_sym(char *name, size_t ind, Context *c) {
    SymTable st = ((SymTable *) c->st_array->arr)[ind];
    Symbol *dst;
    ErrCode e = get_dict(&name, &dst, st.dict);
    while (e == KEY_NOT_FOUND) {
        ind = st.parent_ind;
        if (ind == SIZE_MAX) {
            LOG_DBG(
                "Variable %s does not exist in any scope.", name
            );
            return NULL;
        }
        st = ((SymTable *) c->st_array->arr)[ind];
        e = get_dict(&name, &dst, st.dict);
    }
    return dst;
}

/* Create a new symbol in arena */
Symbol *newsym(char *name, Type *type, Context *c) {
    Symbol s;
    s.name = name;
    s.type = type;
    s.ind = c->symbol_arena->total_occupied;
    s.is_temp = false;
    ArenaRef aref = push_arena(&s, 1, c->symbol_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to push symbol to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

/* Create a new temporary in arena */
Symbol *newtemp(Type *type, Context *c) {
    Symbol t;
    t.name = NULL;
    t.type = type;
    t.ind = c->symbol_arena->total_occupied;
    t.is_temp = true;
    ArenaRef aref = push_arena(&t, 1, c->symbol_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to push temporary to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

/* Create a new symbol table in the symbol table array entry */
ErrCode sym_table(size_t parent_ind, Context *c) {
    ErrCode e;
    SymTable st;
    st.dict = dict(
        sizeof(char *), alignof(char *),
        sizeof(Symbol *), alignof(Symbol *),
        str_intern_hash, str_intern_comp
    );
    if (!st.dict) {
        LOG_ERR("Failed to create symbol table dict.");
        return MEMORY_ALLOC;
    }
    st.parent_ind = parent_ind;

    return push_array(&st, c->st_array);
}

size_t str_intern_hash(const void *key, const size_t cap) {
    uintptr_t x = (uintptr_t) *(char **) key;

    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;

    return ((size_t) x) & (cap - 1);
}

bool str_intern_comp(const void *a, const void *b) {
    return *(char **) a == *(char **) b;
}

/* Function to free symbol table array object. */
void free_sym_table_array_obj(void *item) {
    SymTable *s = (SymTable *) item;
    if (s->dict) free_dict(s->dict);
}

/* Label */

Label *newlabel(Context *c) {
    Label n;
    n.status = CREATED;
    n.ind = c->label_arena->total_occupied;
    ArenaRef aref = push_arena(&n, 1, c->label_arena);
    if (aref.code) {
        LOG_ERR(
            "Failed to push label to arena. Error code: %d.",
            aref.code
        );
        return NULL;
    }
    return aref.ref;
}

/* Operands */

Operand bool_operand(bool val) {
    Operand o;
    o.id = CONST;
    o.constant.type_id = T_BOOL;
    o.constant.boo = val;
    return o;
}

Operand num_operand(int val) {
    Operand o;
    o.id = CONST;
    o.constant.type_id = T_INT;
    o.constant.num = val;
    return o;
}

Operand float_operand(float val) {
    Operand o;
    o.id = CONST;
    o.constant.type_id = T_FLOAT;
    o.constant.flo = val;
    return o;
}

Operand var_operand(Symbol *var) {
    Operand o;
    o.id = VAR;
    o.symbol = var;
    return o;
}

Operand temp_operand(Symbol *t) {
    Operand o;
    o.id = TEMP;
    o.symbol = t;
    return o;
}

Operand err_operand(void) {
    Operand o;
    o.id = ERR_OPERAND;
    return o;
}
