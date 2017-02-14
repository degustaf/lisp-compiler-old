#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "Reader.h"

#include "Numbers.h"
#include "Strings.h"

lisp_object EOF_lisp_object = {EOF_type, NULL, NULL};
lisp_object DONE_lisp_object = {DONE_type, NULL, NULL};
lisp_object NOOP_lisp_object = {NOOP_type, NULL, NULL};

typedef lisp_object* (*MacroFn)(FILE* /* *lisp_object opts, *lisp_object pendingForms */);
static MacroFn macros[128];

// Static Function Declarations.
static MacroFn get_macro(int ch);
static bool isMacro(int ch);
static bool isTerminatingMacro(int ch);
static lisp_object *ReadNumber(FILE *input, char ch);
static char *ReadToken(FILE *input, char ch);
static lisp_object *interpretToken(char *token);         // Needs to be implemented.

// Macros
static lisp_object *CharReader(FILE* input /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *StringReader(FILE* input /* *lisp_object opts, *lisp_object pendingForms */);

void init_macros() {
    macros['\\'] = &CharReader;
    macros['"']  = &StringReader;
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
            lisp_object *ret = (*macro)(input);
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

        if(isdigit(ch)) {
            return ReadNumber(input, ch);
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
            if(ch2 == EOF || isspace(ch2) || isMacro(ch2)) {
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
            if(ch2 == EOF || isspace(ch2) || isTerminatingMacro(ch2)) {
                ungetc(ch2, input);
                buffer[i] = '\0';
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

static lisp_object *CharReader(FILE* input /* *lisp_object opts, *lisp_object pendingForms */) {
    char ch = getc(input);
    if(ch == EOF) {
        fprintf(stderr, "EOF while reading character");
        return &EOF_lisp_object;
    }

    char *token = ReadToken(input, ch);
    if(strlen(token) == 1)
        return (lisp_object*)NewChar(token[0]);
    if(strcmp(token, "newline") == 0)
        return (lisp_object*)NewChar('\n');
    if(strcmp(token, "space") == 0)
        return (lisp_object*)NewChar(' ');
    if(strcmp(token, "tab") == 0)
        return (lisp_object*)NewChar('\t');
    if(strcmp(token, "backspace") == 0)
        return (lisp_object*)NewChar('\b');
    if(strcmp(token, "formfeed") == 0)
        return (lisp_object*)NewChar('\f');
    if(strcmp(token, "return") == 0)
        return (lisp_object*)NewChar('\r');

    fprintf(stderr, "Unsupported character: \\%s", token);
    return NULL;
}

static lisp_object *StringReader(FILE* input /* *lisp_object opts, *lisp_object pendingForms */) {
    size_t size = 256;
    size_t i = 0;
    char *str = malloc(size * sizeof(*str));

    for(int ch = getc(input); ch != '"'; ch = getc(input)) {
        if(ch == EOF) {
            fprintf(stderr, "EOF while reading string.");
            return &EOF_lisp_object;
        }
        if(ch == '\\') {
            ch = getc(input);
            if(ch == EOF) {
                fprintf(stderr, "EOF while reading string.");
                return &EOF_lisp_object;
            }

            switch(ch) {
            case 't':
                ch = '\t';
                break;
            case 'r':
                ch = '\r';
                break;
            case 'n':
                ch = '\n';
                break;
            case 'b':
                ch = '\b';
                break;
            case 'f':
                ch = '\f';
                break;
            case '\\':
            case '"':
                break;
            default:
                fprintf(stderr, "Unsupported Escape character: \\%c", ch);
            }
        }
        if(i + 1 == size) {
            size *= 2;
            void *ptr = realloc(str, size);
            if(ptr == NULL) {
                fprintf(stderr, "Error: %s", strerror(errno));
                return &DONE_lisp_object;
            }
            str = ptr;
        }
        str[i++] = ch;
    }
    return (lisp_object*)NewString(str);
}
