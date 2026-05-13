/* Lexer */
#ifndef H_LEXER
#define H_LEXER

#include "context.h"

/* APIs */
Token scan(char *input, Context *c);

void match(Context *c);

#endif
