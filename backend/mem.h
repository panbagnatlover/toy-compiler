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
            Variable *pt;
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
