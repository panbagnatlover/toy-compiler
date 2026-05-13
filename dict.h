/* Hash map with open addressing.
 * Has no need for `remove`, so no morgue.
 * Key cannot be NULL.
 */
#ifndef H_DICT
#define H_DICT

#include <stdbool.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>
#include "error.h"

#define DEFAULT_BUCKET_SIZE 8
#define RESIZE_UP_LOAD_FACTOR 0.75
#define MAX_ALIGNMENT sizeof(max_align_t)

typedef size_t (*HashFunc)(const void *key, const size_t cap);
typedef bool (*CompFunc)(const void *a, const void *b);

typedef struct Dict {
    void *arr;             // Array of buckets
    size_t occupied;        // Number of buckets occupied
    size_t cap;             // Number of buckets
    size_t node_size;       // Node size (incl. padding)
    // Key metadata
    size_t k_size;          // Byte size of key
    size_t k_align;         // Alignment of key
    size_t k_offset;        // Offset of key in node
    // Value metadata
    size_t v_size;          // Byte size of value
    size_t v_align;         // Alignment of value
    size_t v_offset;        // Offset of value in node
    HashFunc hash_func;     // Hash function
    CompFunc comp_func;  // Key comparison function
} Dict;

/* APIs */

Dict *dict(
    size_t k_size, size_t k_align,
    size_t v_size, size_t v_align,
    HashFunc hash_func, CompFunc comp_func
);
ErrCode put_dict(void *key, void *value, Dict *dict);
ErrCode get_dict(void *key, void *dst, Dict *dict);
void free_dict(Dict *dict);

#endif
