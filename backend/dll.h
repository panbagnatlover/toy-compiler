/* Doubly linked list.
 * TODO: think of a better way for safety. */
#ifndef H_DDL
#define H_DDL

#include <stdlib.h>

typedef struct {
    DLLElem *next;
    DLLElem *prev;
} DLLElem;

typedef struct {
    DLLElem *head;
    DLLElem *tail;
    size_t size;
} DLL;

DLL *dll(void);
DLLElem *dll_elem(void);
void free_dll(DLL *dll);

/* Push a DLL elem to the back of the array. */
void push_dll_elem(DLLElem *elem, DLL *dll) {
    DLLElem *tail = dll->tail;
    if (tail) tail->next = elem;
    elem->prev = tail;
    dll->tail = elem;
    dll->size++;
}

/* Push a DLL array to the back of another DLL array. */
void push_dll(DLL *arr, DLL *dll) {
    DLLElem *tail = dll->tail;
    if (arr->head) arr->head->prev = tail;
    if (tail) tail->next = arr->head;
    dll->size += arr->size;
    free_dll(arr);
}

/* Insert a DLL array in another DLL array,
 * at the location of elem. */
void merge_dll(DLL *arr, DLLElem *elem, DLL *dll) {
    if (!elem) {
        // TODO - insert dll at front
    }

    DLLElem *next = elem->next;
    arr->head->prev = elem;
    elem->next = arr->head;
    arr->tail->next = next;
    if (next) next->prev = arr->tail;

    dll->size += arr->size;
    free_dll(arr);
}

/* Insert new obj at the location of obj. */
void insert(void *obj, void *new_obj) {
    prev_slot = slot->prev;
    next_slot = slot;
    slot = old_slot;
    slot->prev = prev_slot;
    slot->next = next_slot;
    if (prev_slot) prev_slot->next = slot;
    if (next_slot) next_slot->prev = slot;
}

/* remove an obj. */
void remove(void *obj) {
    /* Example: Move old slot to the location of slot. */
    Slot *prev_slot = old_slot->prev;
    Slot *next_slot = old_slot->next;
    if (prev_slot) prev_slot->next = next_slot;
    if (next_slot) next_slot->prev = prev_slot;
}

#endif
