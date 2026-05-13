#ifndef H_ERROR
#define H_ERROR

typedef enum {
    NO_ERR = 0,
    MEMORY_ALLOC,
    SIZE_MISMATCH,
    KEY_NOT_FOUND,
    POP_EMPTY_ARRAY,
    INTERNAL_ERR,
    ABORTED,
    BUG_DETECTED
} ErrCode;

#endif
