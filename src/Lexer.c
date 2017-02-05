#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "Lexer.h"

static token getnumtok(char sign, char first_digit, FILE *input);

token gettok(FILE *input) {
    int last_char = getc(input);
    token ret = {TOKEN_SYMBOL, {.c=0}};

    while(isspace(last_char)){
        last_char = getc(input);
    }

    if(last_char == EOF) {
        ret.type = TOKEN_EOF;
        ret.value.i = last_char;
        return ret;
    }

    if(isalpha(last_char)) {
        int i = 0;
        ret.type = TOKEN_IDENTIFIER;
        ret.value.str[i] = last_char;
        while(isalnum(last_char = getc(input))) {
            i++;
            ret.value.str[i] = last_char;
        }
        ungetc(last_char, input);
        return ret;
    }

    if(last_char == '+' || last_char == '-') {
        char prev = last_char;

        last_char = getc(input);
        if(isdigit(last_char))
            return getnumtok(prev, last_char, input);
        ungetc(last_char, input);

        ret.value.c = prev;
        return ret;
    }

    if(isdigit(last_char)) 
        return getnumtok(' ', last_char, input);

    ret.value.c = last_char;
    return ret;
}

int isodigit(int c) {
    return (c >= '0') && (c <= '7');
}

static token getnumtok(char sign, char first_digit, FILE *input) {
    char buffer[MAX_TOKEN_LENGTH] = "";
    token ret = {TOKEN_INTEGER, {.i=0}};
    int (*f)(int) = &isdigit;
    char *endptr = NULL;
    int i = 2;
    char last_char;

    buffer[0] = sign;
    buffer[1] = first_digit;

    if(first_digit == '0') {
        last_char = getc(input);
        if((last_char == 'x') || (last_char == 'X')) {
            f = &isxdigit;
            buffer[i++] = last_char;
        } else {
            f =  &isodigit;
            ungetc(last_char, input);
        }
    }
    for(; (*f)(last_char = getc(input)); i++)
        buffer[i] = last_char;
    ret.value.i = strtol(buffer, &endptr, 0);
    if(errno) {
        int err = errno;
        fprintf(stderr, "Tokenization Error: %s\n", strerror(err));
    }

    for(ungetc(last_char, input); buffer + i > endptr; i--)
        ungetc(buffer[i], input);
    return ret;
}
