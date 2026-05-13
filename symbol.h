/* Symbol table */

#ifndef H_SYMBOL
#define H_SYMBOL

#include <stdbool.h>
#include "type.h"
#include "dict.h"

typedef struct {
    int ind;
    bool is_temp;
    char *name;
    Type *type;
} Symbol;

typedef struct {
    Dict *dict;
    size_t parent_ind;
} SymTable;

#endif
