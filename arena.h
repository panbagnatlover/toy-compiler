/* Arena.
 * Generic type. Once item added to arena,
 * their references do not change.
 */

#ifndef H_ARENA
#define H_ARENA

#include <stdlib.h>
#include "error.h"

typedef struct ArenaBlock {
    size_t occupied;
    void *arr;
    struct ArenaBlock *next;
} ArenaBlock;

/* Function definition to free an arena block. */
typedef void (*FreeArenaBlockFunc)(ArenaBlock *block);

typedef struct {
    size_t block_size;
    size_t elem_width;
    size_t total_occupied;
    ArenaBlock *head;
    ArenaBlock *tail;
    FreeArenaBlockFunc free_block_func;
} Arena;

/* Arena return object with reference to arena
 * item and error code */
typedef struct {
    void *ref;
    ErrCode code;
} ArenaRef;

/* APIs */

Arena *arena(size_t elem_width, size_t block_size, FreeArenaBlockFunc func);

ArenaRef push_arena(void *item, size_t units, Arena *a);

void free_arena(Arena *a);

void free_arena_block(ArenaBlock *block);

#endif
