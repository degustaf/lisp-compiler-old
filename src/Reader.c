#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "Reader.h"

#include "Numbers.h"

lisp_object EOF_lisp_object = {EOF_type, NULL};
lisp_object DONE_lisp_object = {DONE_type, NULL};
lisp_object NOOP_lisp_object = {NOOP_type, NULL};

typedef lisp_object* (*MacroFn)(FILE*, char /* *lisp_object opts, *lisp_object pendingForms */);
static MacroFn macros[128];

// Static Function Declarations.
static MacroFn get_macro(int ch);
static bool isMacro(int ch);
static bool isTerminatingMacro(int ch);
static lisp_object *ReadNumber(FILE *input, char ch);
static char *ReadToken(FILE *input, char ch);
static lisp_object *interpretToken(char *token);         // Needs to be implemented.

void init_macros() {
}

lisp_object *read(FILE *input, bool EOF_is_error, char return_on /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */) {
    int ch = getc(input);

    while(true) {
        while(isspace(ch)){
            ch = getc(input);
        }
    
        if(ch == EOF) {
            if(EOF_is_error) {
                fprintf(stderr, "EOF while reading");
            }
            return &EOF_lisp_object;
        }
    
        if(ch == return_on) {
            return &DONE_lisp_object;
        }
    
        MacroFn macro = get_macro(ch);
        if(macro) {
            lisp_object *ret = (*macro)(input, ch);
            if(ret->type == NOOP_type) {
                continue;
            }
            return ret;
        }
    
        if(ch == '+' || ch == '-') {
            int ch2 = getc(input);
            if(isdigit(ch2)) {
                ungetc(ch2, input);
                return ReadNumber(input, ch);
            }
        }

        char* token = ReadToken(input, ch);
        lisp_object *ret = interpretToken(token);
        free(token);
        return ret;
    }
}

static MacroFn get_macro(int ch) {
    if((size_t)ch < sizeof(macros)/sizeof(*macros)) {
        return macros[ch];
    }
    return NULL;
}

static bool isMacro(int ch) {
    return ((size_t)ch < sizeof(macros)/sizeof(*macros)) && (macros[ch] != NULL);
}

static bool isTerminatingMacro(int ch) {
    return ch != '#' && ch != '\'' && ch != '%' && isMacro(ch);
}

static lisp_object* ReadNumber(FILE *input, char ch) {
    size_t i = 1;
    size_t size = 256;
    char *buffer = malloc(size * sizeof(*buffer));

    for(buffer[0] = ch; true; size *= 2) {
        for( ;i<size; i++) {
            int ch2 = getc(input);
            if(ch == EOF || isspace(ch) || isMacro(ch)) {
                char *endptr = NULL;
                errno = 0;
                buffer[i] = '\0';
                long ret_l = strtol(buffer, &endptr, 0);
                if(errno == 0 && endptr == buffer + i) {
                    lisp_object *ret = (lisp_object*) NewInteger(ret_l);
                    free(buffer);
                    return ret;
                }
                errno = 0;
                double ret_d = strtod(buffer, &endptr);
                if(errno == 0 && endptr == buffer + i) {
                    lisp_object *ret = (lisp_object*) NewFloat(ret_d);
                    free(buffer);
                    return ret;
                }
                fprintf(stderr, "Invalid number: %s\n", buffer);
                return NULL;
            }
            buffer[i] = ch2;
        }
        buffer = realloc(buffer, 2*size);
    }
}

static char *ReadToken(FILE *input, char ch) {
    size_t i = 1;
    size_t size = 256;
    char *buffer = malloc(size * sizeof(*buffer));

    for(buffer[0] = ch; true; size *= 2) {
        for( ;i<size; i++) {
            int ch2 = getc(input);
            if(ch == EOF || isspace(ch) || isTerminatingMacro(ch)) {
                ungetc(ch, input);
                return buffer;
            }
            buffer[i] = ch2;
        }
        buffer = realloc(buffer, 2*size);
    }
}

static lisp_object *interpretToken(char *token) {
    return NULL;
}
