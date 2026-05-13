/* Lexer.
 * Has scan() and match() methods. scan()
 * recognizes the next token from one of the tags
 * by moving t_end. match() moves t_begin to t_end.
 * 
 * Error-handling pattern:
 * Return ERR_TOKEN. Skip the character(s) causing
 * the invalid pattern for the next scan.
 * 
 * TODO: overflow
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "lexer.h"
#include "parser.h"
#include "context.h"
#include "utils.h"
#include "log.h"

#define STR_BUFFER_SIZE 1028  // Same as string arena block size

bool is_space(char w) {
    return w == ' ' || w == '\t' || w == '\n';
}

bool is_alpha(char w) {
    return (w >= 'a' && w <= 'z') ||
        (w >= 'A' && w <= 'Z') ||
        (w == '_');
}

bool is_alphanumeric(char w) {
    return (w >= 'a' && w <= 'z') ||
        (w >= 'A' && w <= 'Z') ||
        (w >= '0' && w <= '9') ||
        (w == '_');
}

bool is_numeric(char w) {
    return (w >= '0' && w <= '9') || w == '.';
}

bool is_digit(char w) {
    return w >= '0' && w <= '9';
}

/* Move the start pointer forward to the next token */
void match(Context *c) {
    c->t_begin = c->t_end;
}

/* Remove leading space */
void remove_space(char *input, Context *c) {
    if (is_space(input[c->t_begin])) {
        do {
            if (input[c->t_end] == '\n') c->t_line++;
            c->t_end++;
        } while (is_space(input[c->t_end]));
        c->t_begin = c->t_end;
    }
}

/* Remove leading comments */
void remove_comments(char *input, Context *c) {
    while (
        input[c->t_begin] == '/' && input[c->t_begin + 1] &&
        (input[c->t_begin + 1] == '*' || input[c->t_begin + 1] == '/')
    ) {
        c->t_end++;

        /* Multi-line comments */
        if (input[c->t_end] == '*') {
            c->t_end++;

            while (
                input[c->t_end] && input[c->t_end + 1] &&
                (input[c->t_end] != '*' || input[c->t_end + 1] != '/')
            ) {
                if (input[c->t_end] == '\n') c->t_line++;
                c->t_end++;
            }

            /* If the comment is not properly closed
             * at EOF, close it */
            if (!input[c->t_end]) {
                c->t_begin = c->t_end;
                return;
            }
            else if (!input[c->t_end + 1]) {
                c->t_end++;
                c->t_begin = c->t_end;
                return;
            }

            /* Skip the comment */
            c->t_end += 2;
            c->t_begin = c->t_end;
        }

        /* Single-line comments */
        else if (input[c->t_end] == '/') {
            do { c->t_end++; } while (
                input[c->t_end] && input[c->t_end] != '\n'
            );

            /* If the comment is not properly closed
             * at EOF, close it */
            if (!input[c->t_end]) {
                c->t_begin = c->t_end;
                return;
            }

            c->t_line++;

            /* Skip the comment */
            c->t_end++;
            c->t_begin = c->t_end;
        }

        // Remove any leading space
        remove_space(input, c);
    }
}

/* Each scan increments character index.
 * Returns NULL if EOF reached. */
Token scan(char *input, Context *c) {
    /* Return current token if start and end
     * pointers are already in place */
    if (c->t_begin != c->t_end) return c->curr_token;

    Token token;

    /* Remove any leading space */
    remove_space(input, c);

    /* Remove any leading comments */
    remove_comments(input, c);

    char head = input[c->t_begin];

    /* Return EOF marker if EOF reached */
    if (!head) {
        LOG_DBG("EOF reached. Returning EOF token.");
        token.token_id = EOF_TOKEN;
        return token;
    }

    /* Handle alphanumeric tokens */
    if (is_alpha(head)) {
        char buffer[STR_BUFFER_SIZE]; size_t len = 0;
        /* Greedily match alphanumeric symbols */
        do {
            /* If string is longer than buffer cap,
             * return error. Next scan() will
             * start where it errored out. */
            if (len == STR_BUFFER_SIZE - 1) {
                EMIT_SYNTAX_ERR(
                    c->t_line,
                    "Token length is longer than buffer."
                );
                c->t_begin = c->t_end;
                token.token_id = ERR_TOKEN;
                return token;
            }
            buffer[len++] = input[c->t_end++];
        } while (is_alphanumeric(input[c->t_end]));

        /* Add \0 to mark the end of string */
        buffer[len++] = '\0';

        token = get_str_token(buffer, len, c);
    }

    /* Handle numeric tokens that starts with a number */
    else if (is_numeric(head)) {
        int i = 0;
        float f = 0;
        int s = 0;

        /* Leading with digits */
        if (is_digit(input[c->t_begin])) {
            /* Greedily match digits */
            do {
                i = i * 10 + input[c->t_end] - '0';
                c->t_end++;
            } while (
                input[c->t_end] && is_digit(input[c->t_end])
            );

            /* Recognize if the current token is a float */
            if (input[c->t_end] == '.') {
                c->t_end++;
                int t = 10;
                do {
                    f += (float) (input[c->t_end] - '0') / t;
                    t *= 10;
                    c->t_end++;
                } while (is_digit(input[c->t_end]));
            }
        }
        /* Leading with floating point */
        else {
            /* Return ERR if `.` is standalone */
            if (!is_digit(input[c->t_end + 1])) {
                EMIT_SYNTAX_ERR(
                    c->t_line, "Unrecognized token: `.`."
                );
                c->t_end++; c->t_begin = c->t_end;  /* Skip invalid token */
                token.token_id = ERR_TOKEN;
                return token;
            }

            c->t_end++;
            int t = 10;
            do {
                f += (float)(input[c->t_end] - '0') / t;
                t *= 10;
                c->t_end++;
            } while (is_digit(input[c->t_end]));
        }

        /* Recognize any scientific notation */
        if (input[c->t_end] == 'e') {
            if (is_digit(input[c->t_end + 1])) {
                c->t_end++;
                do {
                    s = s * 10 + c->t_end - '0';
                    c->t_end++;
                } while (is_digit(input[c->t_end]));
            }
            else if (input[c->t_end + 1] == '-' && is_digit(input[c->t_end + 2])) {
                c->t_end += 2;
                do {
                    s = s * 10 + input[c->t_end] - '0';
                    c->t_end++;
                } while (is_digit(input[c->t_end]));
            }
        }

        /* If parsed number is a scientific notation */
        if (s) {
            token.token_id = FLO;
            token.flo_val = (i + f) * powf(10.0, s);
        }
        else if (f) {
            token.token_id = FLO;
            token.flo_val = i + f;
        }
        else {
            token.token_id = NUM;
            token.num_val = i;
        }
    }

    /* Handle other head characters */
    else {
        switch (head) {
            case '+':
                c->t_end++;
                token.token_id = PLU;
                break;
            case '-':
                c->t_end++;
                /* Tag '-' as minus, and let the
                 * parser decide on whether it's
                 * minus or unary */
                token.token_id = MIN;
                break;
            case '*':
                c->t_end++;
                token.token_id = MUL;
                break;
            case '/':
                c->t_end++;
                token.token_id = DIV;
                break;
            case '=':
                c->t_end++;
                if (input[c->t_end] == '=') {
                    c->t_end++;
                    token.token_id = EQL;
                }
                else token.token_id = ASG;
                break;
            case '!':
                c->t_end++;
                if (input[c->t_end] == '=') {
                    c->t_end++;
                    token.token_id = NEQ;
                }
                else token.token_id = NOT;
                break;
            case '>':
                c->t_end++;
                if (input[c->t_end] == '=') {
                    c->t_end++;
                    token.token_id = GEQ;
                }
                else token.token_id = GE;
                break;
            case '<':
                c->t_end++;
                if (input[c->t_end] == '=') {
                    c->t_end++;
                    token.token_id = LEQ;
                }
                else token.token_id = LE;
                break;
            case '&':
                c->t_end++;
                if (input[c->t_end] == '&') {
                    c->t_end++;
                    token.token_id = AND;
                }
                else {
                    EMIT_SYNTAX_ERR(
                        c->t_line,
                        "Unrecognized token: `&%c`.",
                        input[c->t_end]
                    );
                    c->t_begin = c->t_end;
                    token.token_id = ERR_TOKEN;
                }
                break;
            case '|':
                c->t_end++;
                if (input[c->t_end] == '|') {
                    c->t_end++;
                    token.token_id = OR;
                }
                else {
                    EMIT_SYNTAX_ERR(
                        c->t_line,
                        "Unrecognized token: `|%c`.",
                        input[c->t_end]
                    );
                    c->t_begin = c->t_end;
                    token.token_id = ERR_TOKEN;
                }
                break;
            case '(':
                c->t_end++;
                token.token_id = LPR;
                break;
            case ')':
                c->t_end++;
                token.token_id = RPR;
                break;
            case '{':
                c->t_end++;
                token.token_id = LBR;
                break;
            case '}':
                c->t_end++;
                token.token_id = RBR;
                break;
            case '[':
                c->t_end++;
                token.token_id = LSQ;
                break;
            case ']':
                c->t_end++;
                token.token_id = RSQ;
                break;
            case ';':
                c->t_end++;
                token.token_id = SEM;
                break;
            default:
                EMIT_SYNTAX_ERR(
                    c->t_line,
                    "Unrecognized token: `%c`.",
                    head
                );
                c->t_end++; c->t_begin = c->t_end;  /* Skip invalid token */
                token.token_id = ERR_TOKEN;
        }
    }

    c->curr_token = token;
    return token;
}
