/* Global context.
 * 
 * Base data structures and storage behaviors
 * for compiler objects:
 * Token:   Transient object. No storage. A new
 *          token gets created for each new word
 *          scanned, and immediately gets consumed
 *          by the parser.
 * Word:    A tokenizable unit. Some words are never
 *          stored and immediately gets tagged as a
 *          token (e.g., special characters). Other
 *          words - keywords and variables - are
 *          interned in a string arena and retrieved
 *          via a dict API.
 * Type:    Interned in a type arena. Basic types
 *          have hard-coded references in the context
 *          for fast retrieval. Array types are
 *          retrieved via a dict API.
 * Node:    AST nodes. Stored in arena. A new node
 *          is created for each statement / expression
 *          in the source code. Therefore, Nodes can
 *          have duplicate signatures.
 * Symbol:  Variables or compiler-generated temporaries.
 *          Stored in arena. A new symbol is created
 *          for each expression in the source code, if
 *          applicable. Therefore, symbols can have
 *          duplicate signatures.
 * Symbol Table:    Stored in an array. Referenced
 *                  by array index. Each table has
 *                  a dict object for storing symbols
 *                  in the scope it represents.
 * Label:   Stored in arena due to lazy evaluation.
 *          When a label is first created, it is
 *          pushed to an arena. When it's invoked
 *          again for code generation, the label
 *          status is updated.
 * Exit Stack:  Array of references to label arena
 *              objects. The array items themselves
 *              are referenced by array index.
 */

#ifndef H_CONTEXT
#define H_CONTEXT

#include <stdbool.h>
#include "array.h"
#include "arena.h"
#include "dict.h"
#include "type.h"
#include "token.h"

#define ARENA_BLOCK_SIZE 32
#define STR_ARENA_BLOCK_SIZE 1028

/* Brain dump:
 * Token, node objects: create new one as you go
 * Strings for ID and keywords: hash map + arena
 * Symbols: create new one, but has hash map for st
 * ST: array of ST objects
 * Types: hash map + arena
 */
typedef struct {
    bool has_parsing_error;
    bool has_semantic_error;
    bool has_3ac_error;

    int t_begin;  // Start marker of a token
    int t_end;    // End marker of a token
    int t_line;  // Current line of token
    Token curr_token;  // Copy of the current token in lexer

    Arena *str_arena; // String arena
    Dict *str_map;  //  TokenIDLookup map: <char *, { char *, TokenID }>

    Type *int_type;  // Reference to int type in arena
    Type *float_type;  // Reference to float type in arena
    Type *bool_type;  // Reference to bool type in arena

    Arena *type_arena;  // Type arena
    Dict *type_map;  // Type map: <Type, Type *>

    Arena *node_arena;  // Node arena

    Arena *symbol_arena;  // Symbol arena
    Array *st_array;  // Symbol table array

    Arena *label_arena;  // Label arena
    Array *exit_stack;  // Exit label stack

    // TODO: Three-address code array

} Context;

/* APIs */

Context *init_context(void);
void free_context(Context *c);

#endif
