/* Dynamic array.
 * Returns error code.
 */
#ifndef H_ARRAY
#define H_ARRAY

#include <stdbool.h>
#include <stdlib.h>
#include "error.h"

#define DEFAULT_ARRAY_SIZE 8

/* Function definition to free array items. */
typedef void (*FreeArrayObjFunc) (void *item);

typedef struct {
    void *arr;
    size_t cap;
    size_t size;
    size_t elem_size;
    FreeArrayObjFunc free_array_obj_func;
} Array;

/* APIs */

Array *array(size_t elem_size, FreeArrayObjFunc func);
ErrCode push_array(void *item, Array *array);
ErrCode pop_array(Array *array);
void free_array(Array *array);
void free_array_obj_generic(void *item);

#endif
