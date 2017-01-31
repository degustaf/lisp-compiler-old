#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#define MAX_TOKEN_LENGTH 256

typedef enum {
    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_FLOAT,
    TOKEN_SYMBOL,
} token_type;

typedef struct {
    token_type type;
    union {
        char c;
        long i;
        double f;
        char str[MAX_TOKEN_LENGTH];
    } value;
} token;


token gettok(FILE * input);
token getnumtok(char sign, char first_digit, FILE *input);

#endif /* LEXER_H */
