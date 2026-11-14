/* Doubly linked list. */
#ifndef H_DDL
#define H_DDL

#include <stdlib.h>

typedef struct {
    void *elem;
    DLLElem *next;
    DLLElem *prev;
} DLLElem;

typedef struct {
    DLLElem *head;
    DLLElem *tail;
    size_t elem_size;
    size_t size;
} DLL;

DLL *dll(size_t elem_size);
DLLElem *dll_elem(void *elem);

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
