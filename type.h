/* Type. */

#ifndef H_TYPE
#define H_TYPE

#include <stdbool.h>

typedef enum {
    T_INT = 1,
    T_BOOL,
    T_FLOAT,
    T_ARRAY
} TypeID;

typedef struct Type {
    TypeID id;
    int width;
    struct Type *elem;
} Type;

#endif
