/*
    This toy compiler translates language from
    the following grammar to 3-address code:

    program     -> block
    block       -> { decls stmts }
    decls       -> decls decl | e
    decl        -> type id ;
    type        -> type [ num ] | basic
    stmts       -> stmts stmt | e

    stmt        -> loc = expr ;
                 | if ( expr ) stmt
                 | if ( expr ) stmt else stmt
                 | while ( expr ) stmt
                 | do stmt while ( expr ) ;
                 | break ;
                 | block
    loc         -> loc [ expr ] | id ;

    expr        -> expr || join | join
    join        -> join && equality | equality
    equality    -> equality == rel | equality != rel | rel
    rel         -> arith < arith | arith <= arith | arith > arith
                 | arith >= arith | arith
    arith       -> arith + term | arith - term | term
    term        -> term * unary | term / unary | unary
    unary       -> ! unary | - unary | factor
    factor      -> ( expr ) | loc | num | real | true | false
*/

#include <stdio.h>
#include <stdlib.h>
#include "node.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "3ac.h"
#include "context.h"
#include "log.h"

int main(int argc, char **argv) {

    /* I/O */

    if (argc != 2) {
        LOG_ERR("Must specify a filepath.");
        exit(1);
    }

    /* Validates input is a filepath */
    char *filepath = argv[1];

    /* Open file */
    LOG_DBG("Opening file...");
    FILE *f = fopen(filepath, "r");
    if (!f) {
        LOG_ERR("File path %s not found", filepath);
        exit(1);
    }

    /* Read whole file to buffer */
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        LOG_ERR("Failed to seek file. Exit.");
        exit(1);
    }

    long fsize = ftell(f);
    if (fsize < 0) {
        fclose(f);
        LOG_ERR("Failed to determine file size. Exit.");
        exit(1);
    }
    rewind(f);

    char *input = malloc((size_t) fsize + 1);
    if (!input) {
        fclose(f);
        LOG_ERR("Failed to allocate memory for file content. Exit.");
        exit(1);
    }

    size_t read = fread(input, 1, fsize, f);
    if (read != (size_t) fsize) {
        free(input);
        fclose(f);
        LOG_ERR("Failed to read whole file. Exit.");
        exit(1);
    }
    input[fsize] = '\0';
    fclose(f);

    LOG_DBG("Read %zu bytes of source code. Initializing context.", read);

    /* Initialize context */
    Context *c = NULL;
    c = init_context();
    if (!c) goto cleanup;

    LOG_DBG("Context initialized. Start parsing.");

    /* Parsing */
    Node *node = program_parser(input, c);

    LOG_DBG("Parser ran with error code %d. Start semantic analysis.", c->has_parsing_error);

    /* Semantic analysis */
    ErrCode e = program_analyzer(node, c);
    if (e) goto cleanup;

    LOG_DBG("Semantic analyzer ran with error code %d.", c-> has_semantic_error);

    /* Three-address code translation */
    if (c->has_parsing_error || c->has_semantic_error) goto cleanup;

    LOG_DBG("Start translation.");

    e = block_3ac(node, c);
    if (e) goto cleanup;

    LOG_DBG("Translation complete. Deleting context.");

    free_context(c);
    return 0;

cleanup:
    LOG_DBG("Error occurred. Deleting context.");

    free_context(c);
    return 1;
}
