/* Basic block data structure and functions. */

#ifndef H_BLOCK
#define H_BLOCK

typedef struct IRInst;

typedef struct IRBlock {
    enum {ENTRY, BODY, EXIT} type;
    struct IRInst *head;
    struct IRInst *tail;
    Set<struct IRBlock *> predecessors;
    struct IRBlock *bl0;
    struct IRBlock *bl1;
};

typedef struct MBlock {
    enum {ENTRY, BODY, EXIT} type;
    struct MInst *head;
    struct MInst *tail;
    Set<struct MBlock *> predecessors;
    struct MBlock *bl0;
    struct MBlock *bl1;
    BitSet defs;
    BitSet uses;
    Loop *loop;
}

#endif
