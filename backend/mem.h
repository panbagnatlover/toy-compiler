/* TODO -
 * Introduce a uni-size pointer type in frontend
 * type pointer: size = 8 bytes
 * A variable can have type pointer
 * 
 * In middle end, all non-pointer/array types have pt() = {NAP}.
 * stuff in pt() is Memory. Memory has a base variable + offset or raw addr.
 * Base array assigned a base variable.
 * 
 * */
#ifndef H_MEMORY
#define H_MEMORY

struct Memory {
    enum {
        VAR;
        RAW_LOC;
        PT_OFFSET;
    } type;

    union {
        Variable *var;

        size_t raw_loc;
        
        struct {
            Variable *pt;  // TODO - get rid of, all base assigned a var
            size_t offset;
            int size;
        } pt_offset;
    }
};

typedef enum AliasType {
    MUST_ALIAS = 0,
    MAY_ALIAS,
    NO_ALIAS,
};

#endif
