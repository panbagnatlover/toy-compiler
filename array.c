#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "array.h"
#include "log.h"

/* Helper function to resize the dynamic array. */
ErrCode _resize(Array *array, size_t new_cap) {
    void *tmp = realloc(array->arr, array->elem_size * new_cap);
    if (!tmp) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory for array.",
            array->elem_size * new_cap
        );
        return MEMORY_ALLOC;
    }
    array->arr = tmp;
    array->cap = new_cap;
    return NO_ERR;
}

Array *array(size_t elem_size, FreeArrayObjFunc func) {
    size_t cap = DEFAULT_ARRAY_SIZE;
    void *arr = malloc(cap * elem_size);
    if (!arr) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory to array.",
            cap * sizeof(void *)
        );
        return NULL;
    }

    Array *array = malloc(sizeof(Array));
    if (!array) {
        LOG_ERR(
            "Failed to allocate %zu bytes of memory to array.",
            sizeof(Array)
        );
        free(arr);
        return NULL;
    }

    array->arr = arr;
    array->cap = cap;
    array->size = 0;
    array->elem_size = elem_size;
    array->free_array_obj_func = func;
    return array;
}

/* Push to the top (back) of the array */
ErrCode push_array(void *item, Array *array) {
    /* Resize the array if current size reaches capacity */
    if (array->size == array->cap) {
        ErrCode err = _resize(array, array->cap * 2);
        if (err) return err;
    }

    /* Copy item to the back of the array */
    memcpy((char *) array->arr + array->size * array->elem_size, item, array->elem_size);
    array->size++;

    return NO_ERR;
}

/* Pop the top of array and free heap memory. */
ErrCode pop_array(Array *array) {
    if (!array->size) {
        LOG_ERR("Attempted to pop an empty array.");
        return POP_EMPTY_ARRAY;
    }

    /* Free the top item */
    void *top = (char *) array->arr + array->elem_size * (array->size - 1);
    array->free_array_obj_func(top);
    array->size--;

    /* If array size reaches 1/4 of capacity, and
     * 1/2 of capacity is greater than DEFAULT_ARRAY_SIZE,
     * shrink the capacity by half */
    if (
        array->cap >= (DEFAULT_ARRAY_SIZE * 2) && 
        array->size == array->cap / 4
    ) return _resize(array, array->cap / 2);
    
    return NO_ERR;
}

/* Free array and each array element. */
void free_array(Array *array) {
    if (!array) return;
    if (array->arr) {
        for (int i = 0; i < array->size; i++) {
            array->free_array_obj_func((char *) array->arr + array->elem_size * i);
        }
        free(array->arr); array->arr = NULL;
    }
    free(array);
}

/* Generic free_array_obj_func: do nothing */
void free_array_obj_generic(void *item) {}
