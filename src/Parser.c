#include "Lexer.h"
#include "Numbers.h"
#include "Parser.h"

static Integer *ParseInteger(FILE *input, token *const current);
static Float *ParseFloat(FILE *input, token *const current);
static lisp_object *LogError(const char *str);

lisp_object *Parser(FILE *input, token *const current) {
    switch(current->type) {
    default:
        return LogError("Unknown token when expecting an expression.");
    case TOKEN_INTEGER:
        return (lisp_object*)ParseInteger(input, current);
    case TOKEN_FLOAT:
        return (lisp_object*)ParseFloat(input, current);
    }
}

static Integer *ParseInteger(FILE *input, token *const current) {
    Integer *ret = NewInteger(current->value.i);
    return ret;
}

static Float *ParseFloat(FILE *input, token *const current) {
    Float *ret = NewFloat(current->value.f);
    return ret;
}

static lisp_object *LogError(const char *str) {
    fprintf(stdout, "LogError: %s\n", str);
    return NULL;
}
