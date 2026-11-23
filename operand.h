/* Operands:
 * CONST represents a constant with a basic type.
 * VAR represents a variable in the var table.
 * TEMP represents a compiler-generated temporary. Also has Var type.
 */

#ifndef H_OPERAND
#define H_OPERAND

#include "type.h"
#include "var.h"

typedef struct {
    enum {CONST, VAR, TEMP, ERR_OPERAND} id;  // TODO: rename to type or something
    union {
        Var *var;

        struct {
            TypeID type_id;

            union {
                int num;
                float flo;
                bool boo;
            };
        } constant;
    };
} Operand;

#endif
