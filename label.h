/* Label. */
#ifndef H_LABEL
#define H_LABEL

typedef struct {
    enum {CREATED, FILLED} status;
    int ind;
} Label;

#endif
