
#include "array.h"
#include "arena.h"
#include "ast.h"
#include "dict.h"
#include "type.h"
#include "token.h"
#include "node.h"
#include "symbol.h"
#include "label.h"
#include "utils.h"
#include "log.h"

Context *init_context(void) {
    ErrCode e;
    Context *c = NULL;
    Arena *str_arena = NULL;
    Arena *type_arena = NULL;
    Arena *node_arena = NULL;
    Arena *symbol_arena = NULL;
    Arena *label_arena = NULL;

    Array *exit_stack = NULL;
    Array *st_array = NULL;

    Dict *str_map = NULL;
    Dict *type_map = NULL;

    c = malloc(sizeof(Context));
    if (!c) goto cleanup;

    c->has_parsing_error = false;
    c->has_semantic_error = false;

    c->t_begin = 0;
    c->t_end = 0;
    c->t_line = 1;  // Starts from line 1

    LOG_DBG("Initialize string arena.");

    // Initialize string arena
    str_arena = arena(sizeof(char), STR_ARENA_BLOCK_SIZE, free_arena_block);
    if (!str_arena) goto cleanup;

    LOG_DBG("Initialize string map.");

    str_map = dict(
        sizeof(char *), alignof(char *),
        sizeof(TokenIDLookup), alignof(TokenIDLookup),
        str_hash, str_comp
    );
    if (!str_map) goto cleanup;

    LOG_DBG("Initialize type arena.");

    // Initialize types
    type_arena = arena(sizeof(Type), ARENA_BLOCK_SIZE, free_arena_block);
    if (!type_arena) goto cleanup;

    LOG_DBG("Initialize type map.");

    type_map = dict(
        sizeof(Type), alignof(Type),
        sizeof(Type *), alignof(Type *),
        type_hash, type_comp
    );
    if (!type_map) goto cleanup;

    LOG_DBG("Initialize node arena.");

    // Intialize node arena
    node_arena = arena(sizeof(Node), ARENA_BLOCK_SIZE, free_node_block);
    if (!node_arena) goto cleanup;

    LOG_DBG("Initialize symbol arena.");

    // Initialize symbol arena
    symbol_arena = arena(sizeof(Symbol), ARENA_BLOCK_SIZE, free_arena_block);
    if (!symbol_arena) goto cleanup;

    LOG_DBG("Initialize label arena.");

    // Initialize label arena
    label_arena = arena(sizeof(Label), ARENA_BLOCK_SIZE, free_arena_block);
    if (!label_arena) goto cleanup;

    LOG_DBG("Initialize symbol table array.");

    // Initialize symbol table array
    st_array = array(sizeof(SymTable), free_sym_table_array_obj);
    if (!st_array) goto cleanup;

    LOG_DBG("Initialize exit label array.");

    // Initialize exit label array
    exit_stack = array(sizeof(Label), free_array_obj_generic);
    if (!exit_stack) goto cleanup;

    c->str_arena = str_arena;
    c->type_arena = type_arena;
    c->node_arena = node_arena;
    c->symbol_arena = symbol_arena;
    c->label_arena = label_arena;

    c->st_array = st_array;
    c->exit_stack = exit_stack;

    c->str_map = str_map;
    c->type_map = type_map;

    LOG_DBG("Seed basic types.");

    /* Seed type */
    e = seed_basic_type(c);
    if (e) goto cleanup;

    LOG_DBG("Seed keywords.");

    /* Seed keywords */
    e = seed_keywords(c);
    if (e) goto cleanup;

    return c;

cleanup:
    LOG_DBG("Error in initializing context. Cleaning up.");

    free_arena(str_arena);
    free_arena(type_arena);
    free_arena(node_arena);
    free_arena(symbol_arena);
    free_arena(label_arena);

    free_array(exit_stack);
    free_array(st_array);

    free_dict(str_map);
    free_dict(type_map);

    free(c);
    return NULL;
}

void free_context(Context *c) {
    if (!c) return;

    LOG_DBG("Free arenas.");

    free_arena(c->str_arena);
    free_arena(c->type_arena);
    free_arena(c->node_arena);
    free_arena(c->symbol_arena);
    free_arena(c->label_arena);

    LOG_DBG("Free arrays.");

    free_array(c->exit_stack);
    free_array(c->st_array);

    LOG_DBG("Free dicts.");

    free_dict(c->str_map);
    free_dict(c->type_map);

    LOG_DBG("Free context.");

    free(c);
}
