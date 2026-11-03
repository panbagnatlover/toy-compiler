#ifndef H_LOOP
#define H_LOOP

typedef struct {
    bool modified;  // When modified, need to reconstruct from head
    MBlock *head;
    Set<MBlock *>blocks;  // TODO: sorted array of ind, or bitset?
    Array<MBlock *>latches;  // TODO: bitset
    Array<MBlock *>exits;
    Loop *parent;
    Loop *children;
} Loop;

#endif
