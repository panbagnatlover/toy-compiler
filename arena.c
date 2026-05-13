#include <stdio.h>
#include <string.h>
#include "log.h"
#include "arena.h"

Arena *arena(size_t elem_width, size_t block_size, FreeArenaBlockFunc func) {
    void *arr = malloc(elem_width * block_size);
    if (!arr) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for arena.",
            elem_width * block_size
        );
        return NULL;
    }

    ArenaBlock *b = malloc(sizeof(ArenaBlock));
    if (!b) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for arena.",
            sizeof(ArenaBlock)
        );
        free(arr);
        return NULL;
    }
    b->arr = arr;
    b->next = NULL;
    b->occupied = 0;

    Arena *a = malloc(sizeof(Arena));
    if (!a) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for arena.",
            sizeof(Arena)
        );
        free(arr);
        free(b);
        return NULL;
    }

    a->head = b;
    a->tail = b;
    a->block_size = block_size;
    a->elem_width = elem_width;
    a->total_occupied = 0;
    a->free_block_func = func;
    return a;
}

/* Push item_width units from the location of item to
 * the arena. If the current block does not have enough
 * space to allocate the item, create a new block.
 * Returns the arena reference to the pushed item. */
ArenaRef push_arena(void *item, size_t units, Arena *a) {
    ArenaRef output;
    /* Validate that item_width <= block size */
    if (units > a->block_size) {
        LOG_ERR(
            "Input item (%zu units) exceeded block size (%zu units).",
            units,
            a->block_size
        );
        output.code = SIZE_MISMATCH;
        output.ref = NULL;
        return output;
    }

    /* If current block has enough space, copy to
     * current block */
    size_t remain = a->block_size - a->tail->occupied;
    if (remain >= units) {
        void *dst = (char *) a->tail->arr + a->elem_width * a->tail->occupied;
        memcpy(dst, item, a->elem_width * units);
        output.ref = dst;
        a->tail->occupied += units;
    }
    /* Otherwise, create a new block and copy there */
    else {
        void *arr = malloc(sizeof(a->elem_width * a->block_size));
        if (!arr) {
            fprintf(
                stderr,
                "Failed to allocate %zu bytes of memory for arena.",
                sizeof(a->elem_width * a->block_size)
            );
            output.ref = NULL;
            output.code = MEMORY_ALLOC;
            return output;
        }

        ArenaBlock *b = malloc(sizeof(ArenaBlock));
        if (!b) {
            fprintf(
                stderr,
                "Failed to allocate %zu bytes of memory for arena.",
                sizeof(ArenaBlock)
            );
            free(arr);
            output.ref = NULL;
            output.code = MEMORY_ALLOC;
            return output;
        }
        b->arr = arr;
        b->next = NULL;
        b->occupied = 0;

        a->tail->next = b;
        a->tail = b;

        /* Copy to the beginning of the new block */
        void *dst = b->arr;
        memcpy(dst, item, a->elem_width * units);
        output.ref = dst;
        b->occupied += units;
    }

    a->total_occupied++;

    output.code = NO_ERR;
    return output;
}

void free_arena(Arena *a) {
    if (!a) return;
    ArenaBlock *head = a->head;
    while (head) {
        ArenaBlock *next = head->next;
        a->free_block_func(head);
        head = next;
    }
    free(a);
}

/* Generic function to free arena blocks */
void free_arena_block(ArenaBlock *block) {
    free(block->arr);
    free(block);
}
