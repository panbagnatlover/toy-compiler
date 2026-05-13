#include <string.h>
#include "dict.h"
#include "error.h"
#include "log.h"

/* Helper function to populate node metadata, including
 * the offset memory locations of key and value,
 * and the total node size. */
static inline void _get_node_size(
    Dict *dict,
    size_t k_size,
    size_t k_align,
    size_t v_size,
    size_t v_align
) {
    dict->k_size = k_size;
    dict->k_align = k_align;
    dict->v_size = v_size;
    dict->v_align = v_align;

    /* Get alignment of node elements:
     * max(alignof(bool), alignof(key), alignof(value)) */
    size_t align = alignof(bool);
    align = align > dict->k_align ? align : dict->k_align;
    align = align > dict->v_align ? align : dict->v_align;

    /* Calculate offset of each element */
    size_t end = sizeof(bool);
    /* Calculate start and end locations of dict key */
    dict->k_offset = (end + align - 1) & ~(align - 1);
    end = dict->k_offset + dict->k_size;
    /* Calculate start and end locations of dict value */
    dict->v_offset = (end + align - 1) & ~(align - 1);
    end = dict->v_offset + dict->v_size;
    /* Calculate padding at the end of the node */
    dict->node_size = (end + MAX_ALIGNMENT - 1) & 
        ~(MAX_ALIGNMENT - 1);
}

Dict *dict(
    size_t k_size, size_t k_align,
    size_t v_size, size_t v_align,
    HashFunc hash_func, CompFunc comp_func
) {
    Dict *d = malloc(sizeof(Dict));
    if (!d) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for dict.",
            sizeof(Dict)
        );
        return NULL;
    }

    _get_node_size(d, k_size, k_align, v_size, v_align);

    /* Initialize buckets to NULL because `push_dict` and
     * `get_dict` uses NULL to identify whether a bucket is filled */
    void *arr = calloc(DEFAULT_BUCKET_SIZE, d->node_size);
    if (!arr) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for dict.",
            DEFAULT_BUCKET_SIZE * d->node_size
        );
        free(d);
        return NULL;
    }

    d->arr = arr;
    d->cap = DEFAULT_BUCKET_SIZE;
    d->hash_func = hash_func;
    d->comp_func = comp_func;
    d->occupied = 0;
    return d;
}

/* Helper function to add key-value pair to a dictionary */
ErrCode _put_dict(void *key, void *value, Dict *dict) {
    size_t ind = dict->hash_func(key, dict->cap);
    char *loc = (char *) dict->arr + ind * dict->node_size;
    bool is_occupied = *(bool *) loc;

    /* While occupied, find the next unoccupied slot */
    size_t iter = 0;
    while (is_occupied) {
        /* If node key matches input key, update the value
         * and return */
        if (dict->comp_func(key, loc + dict->k_offset)) {
            memcpy(loc + dict->v_offset, value, dict->v_size);
            return NO_ERR;
        }

        ind++; iter++;

        /* Break out of the loop if traversed all slots */
        if (iter == dict->cap) {
            LOG_ERR("No capacity in dictionary - bug in code.");
            return INTERNAL_ERR;
        }

        /* If reached the end, set ind to the beginning */
        if (ind == dict->cap) ind = 0;

        loc = (char *) dict->arr + ind * dict->node_size;
        is_occupied = *(bool *) loc;
    }

    memcpy(loc + dict->k_offset, key, dict->k_size);
    memcpy(loc + dict->v_offset, value, dict->v_size);
    *(bool *) loc = true;
    dict->occupied++;
    return NO_ERR;
}

/* Add a key-value pair to a dictionary */
ErrCode put_dict(void *key, void *value, Dict *dict) {
    size_t old_cap = dict->cap;
    float load = dict->occupied / (float) old_cap;

    /* If load factor exceeded, resize up */
    if (load >= RESIZE_UP_LOAD_FACTOR) {
        size_t new_cap = old_cap * 2;

        void *arr = calloc(new_cap, dict->node_size);
        if (!arr) {
            LOG_ERR(
                "Failed to allocate %zu bytes of memory to dict.",
                new_cap * dict->node_size
            );
            return MEMORY_ALLOC;
        }

        /* Rehash each node given new capacity */
        for (int i = 0; i < old_cap; i++) {
            char *loc = (char *) dict->arr + i * dict->node_size;
            bool is_occupied = *(bool *) loc;

            if (is_occupied) {
                size_t n_ind = dict->hash_func(loc + dict->k_offset, new_cap);
                char *n_loc = (char *) arr + n_ind * dict->node_size;
                bool n_is_occupied = *(bool *) n_loc;

                /* Find next open slot */
                while (n_is_occupied) {
                    n_ind++;

                    /* If reached the end, set ind to the beginning */
                    if (n_ind == new_cap) n_ind = 0;

                    n_loc = (char *) arr + n_ind * dict->node_size;
                    n_is_occupied = *(bool *) n_loc;
                }

                memcpy(n_loc, loc, dict->node_size);
            }
        }
        free(dict->arr);
        dict->arr = arr;
        dict->cap = new_cap;
    }

    /* Add new node to dict */
    ErrCode e = _put_dict(key, value, dict);
    if (e) return e;
    return NO_ERR;
}

/* Get a value from dictionary given a key. Copy the value
 * to a designated `dst`. */
ErrCode get_dict(void *key, void *dst, Dict *dict) {
    size_t ind = dict->hash_func(key, dict->cap);
    char *loc = (char *) dict->arr + ind * dict->node_size;
    bool is_occupied = *(bool *) loc;

    size_t iter = 0;
    while (is_occupied && !dict->comp_func(key, loc + dict->k_offset)) {
        ind++; iter++;

        /* Break out of the loop if traversed all slots */
        if (iter == dict->cap) {
            LOG_DBG("Checked all nodes. Key not found in dict.");
            return KEY_NOT_FOUND;
        }

        if (ind == dict->cap) ind = 0;

        loc = (char *) dict->arr + ind * dict->node_size;
        is_occupied = *(bool *) loc;
    }

    /* If reached an empty slot, return not found */
    if (!is_occupied) {
        LOG_DBG("Key not found in dict.");
        return KEY_NOT_FOUND;
    }

    void *value = loc + dict->v_offset;
    memcpy(dst, value, dict->v_size);
    return NO_ERR;
}

void free_dict(Dict *dict) {
    if (!dict) return;
    if (dict->arr) free(dict->arr);
    free(dict);
}
