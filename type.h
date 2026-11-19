/* Type. */
/* TODO - add memory type. */

#ifndef H_TYPE
#define H_TYPE

typedef enum {
    T_INT = 1,
    T_BOOL,
    T_FLOAT,
    T_ARRAY
} TypeID;

typedef struct {
    TypeID id;
    int width;
    Type *elem;
} Type;

#endif
