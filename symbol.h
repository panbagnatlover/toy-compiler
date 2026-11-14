/* Symbol table */

#ifndef H_SYMBOL
#define H_SYMBOL

#include <stdbool.h>
#include "type.h"
#include "dict.h"

typedef struct {
    int ind;  // TODO: every var must be indexed
    bool is_temp; /* TODO - may not need this */
    char *name;  // TODO: no need for this
    Type *type;
} Symbol;

typedef struct {
    Dict *dict;
    size_t parent_ind;
} SymTable;

#endif
