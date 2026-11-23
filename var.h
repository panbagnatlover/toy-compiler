/* Variables. Flow insensitive. */

#ifndef H_VAR
#define H_VAR

#include <stdbool.h>
#include "type.h"
#include "dict.h"

typedef struct {
    int ind;  // TODO: every var must be indexed
    bool is_temp; /* TODO - may not need this */
    char *name;  // TODO: no need for this
    Type *type;
    bool stack_allocated;  // Needed on var. Flow insensitive attribute.
} Var;

typedef struct {
    Dict *dict;
    size_t parent_ind;
} SymTable;

#endif
