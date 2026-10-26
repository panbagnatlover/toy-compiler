#ifndef H_LOOP
#define H_LOOP

typedef struct {
    MBlock *head;
    Array<MBlock *>blocks;
    Array<MBlock *>exits;
} Loop;

#endif