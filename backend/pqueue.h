#ifndef H_PQUEUE
#define H_PQUEUE

#include "stdlib.h"

typedef struct {
    size_t size;
} PQueue;

void insert_pqueue(void *elem, PQueue *pq);
void *top_pqueue(PQueue *pq);

#endif
