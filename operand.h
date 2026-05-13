/* Operands:
 * CONST represents a constant with a basic type.
 * VAR represents a variable in the symbol table.
 * TEMP represents a compiler-generated temporary. Also has Symbol type.
 */

#ifndef H_OPERAND
#define H_OPERAND

#include "type.h"
#include "symbol.h"

typedef struct {
    enum {CONST, VAR, TEMP, ERR_OPERAND} id;
    union {
        Symbol *symbol;

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
