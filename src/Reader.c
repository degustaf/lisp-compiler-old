#include "Reader.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.
#include "gc.h"
#include "List.h"
#include "Map.h"
#include "Numbers.h"
#include "Strings.h"
#include "Vector.h"

lisp_object DONE_lisp_object = {DONE_type, sizeof(lisp_object), NULL, NULL, NULL, NULL, NULL};
lisp_object NOOP_lisp_object = {NOOP_type, sizeof(lisp_object), NULL, NULL, NULL, NULL, NULL};

typedef lisp_object* (*MacroFn)(FILE*, char /* *lisp_object opts, *lisp_object pendingForms */);
static MacroFn macros[128];

// Static Function Declarations.
static MacroFn get_macro(int ch);
static bool isMacro(int ch);
static bool isTerminatingMacro(int ch);
static lisp_object *ReadNumber(FILE *input, char ch);
static char *ReadToken(FILE *input, char ch);
static lisp_object *interpretToken(char *token);         // Needs to be implemented.
static const lisp_object **ReadDelimitedList(FILE *input, char delim, size_t *const count /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */);

// Macros
static lisp_object *CharReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *ListReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *VectorReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *MapReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *StringReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static lisp_object *UnmatchedParenReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);

void init_macros() {
    macros['\\'] = CharReader;
    macros['"']  = StringReader;
    macros['(']  = ListReader;
    macros[')']  = UnmatchedParenReader;
    macros['[']  = VectorReader;
    macros[']']  = UnmatchedParenReader;
    macros['{']  = MapReader;
    macros['}']  = UnmatchedParenReader;
}

lisp_object *read(FILE *input, bool EOF_is_error, char return_on /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */) {
    int ch = getc(input);

    while(true) {
        while(isspace(ch)){
            ch = getc(input);
        }

        if(ch == EOF) {
            if(EOF_is_error) {
                return (lisp_object*) NewError(false, "EOF while reading");
            }
            return (lisp_object*) NewError(true, "EOF while reading");
        }

        if(ch == return_on) {
            return &DONE_lisp_object;
        }

        MacroFn macro = get_macro(ch);
        if(macro) {
            lisp_object *ret = macro(input, (char) ch);
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
    char *buffer = GC_MALLOC_ATOMIC(size * sizeof(*buffer));
	assert(buffer);

    for(buffer[0] = ch; true; size *= 2) {
        for( ;i<size; i++) {
            int ch2 = getc(input);
            if(ch2 == EOF || isspace(ch2) || isMacro(ch2)) {
                ungetc(ch2, input);
                char *endptr = NULL;
                errno = 0;
                buffer[i] = '\0';
                long ret_l = strtol(buffer, &endptr, 0);
                if(errno == 0 && endptr == buffer + i) {
                    lisp_object *ret = (lisp_object*) NewInteger(ret_l);
                    return ret;
                }
                errno = 0;
                double ret_d = strtod(buffer, &endptr);
                if(errno == 0 && endptr == buffer + i) {
                    lisp_object *ret = (lisp_object*) NewFloat(ret_d);
                    return ret;
                }
				size_t len = strlen(buffer) + 20;
				char *ErrBuffer = GC_MALLOC_ATOMIC(len * sizeof(*ErrBuffer));
                snprintf(ErrBuffer, len, "Invalid number: %s", buffer);
				lisp_object *ret = (lisp_object*) NewError(false, "Invalid number: %s", buffer);
                return ret;
            }
            buffer[i] = ch2;
        }
        buffer = GC_REALLOC(buffer, 2*size);
		assert(buffer);
    }
}

static char *ReadToken(FILE *input, char ch) {
    size_t i = 1;
    size_t size = 256;
    char *buffer = GC_MALLOC_ATOMIC(size * sizeof(*buffer));
	assert(buffer);

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
        buffer = GC_REALLOC(buffer, 2*size);
		assert(buffer);
    }
}

static lisp_object *interpretToken(char __attribute__((unused)) *token) {
	assert(false /* interpretToken is not implemented. */);
    return NULL;
}

static lisp_object *CharReader(FILE* input, __attribute__((unused)) char c /* *lisp_object opts, *lisp_object pendingForms */) {
    char ch = getc(input);
    if(ch == EOF) {
        return (lisp_object*) NewError(false, "EOF while reading character");
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

    return (lisp_object*) NewError(false, "Unsupported character: \\%s", token);
}

static lisp_object *StringReader(FILE* input, __attribute__((unused)) char c /* *lisp_object opts, *lisp_object pendingForms */) {
    size_t size = 256;
    size_t i = 0;
    char *str = GC_MALLOC_ATOMIC(size * sizeof(*str));
	assert(str);

    for(int ch = getc(input); ch != '"'; ch = getc(input)) {
        if(ch == EOF) {
            return (lisp_object*) NewError(false, "EOF while reading string.");
        }
        if(ch == '\\') {
            ch = getc(input);
            if(ch == EOF) {
            	return (lisp_object*) NewError(false, "EOF while reading string.");
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
                return (lisp_object*) NewError(false, "Unsupported Escape character: \\%c", ch);
            }
        }
        if(i == size) {
            size *= 2;
            str = GC_REALLOC(str, size);
			assert(str);
        }
        str[i++] = ch;
    }
	str[i++] = '\0';
	lisp_object *obj = (lisp_object*) NewString(str);
	return obj;
}

static const lisp_object **ReadDelimitedList(FILE *input, char delim, size_t *const count /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */) {
    size_t size = 8;
    size_t i = 0;
    const lisp_object **ret = GC_MALLOC(size * sizeof(*ret));
	assert(ret);

    while(true) {
        lisp_object *form = read(input, true, delim);
        if(form->type == EOF_type) return NULL;
        if(form == &DONE_lisp_object) {
            *count = i;
            return ret;
        }
        if(size == i) {
            size *= 2;
            ret = GC_REALLOC(ret, size * sizeof(*ret));
			assert(ret);
        }
        ret[i++] = form;
    }
}

static lisp_object *ListReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
    size_t count;
    const lisp_object **list = ReadDelimitedList(input, ')', &count);
    if(list == NULL) return (lisp_object*) NewError(true, "");
    if(count == 0) return (lisp_object*)EmptyList;
    return (lisp_object*)CreateList(count, list);
}

static lisp_object *VectorReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
    size_t count;
    const lisp_object **list = ReadDelimitedList(input, ']', &count);
    if(list == NULL) return (lisp_object*) NewError(true, "");
	return (lisp_object*) CreateVector(count, list);
}

static lisp_object *MapReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
    size_t count;
    const lisp_object **list = ReadDelimitedList(input, '}', &count);
    if(list == NULL) return (lisp_object*) NewError(true, "");
	if(count & 1) return (lisp_object*) NewError(false, "Map literal must contain an even number of forms");
    return (lisp_object*)CreateHashMap(count, list);
}

static lisp_object *UnmatchedParenReader(__attribute__((unused)) FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	return (lisp_object*) NewError(false, "Unmatched delimiter: %c", ch);
}
