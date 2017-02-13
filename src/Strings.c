#include <stdlib.h>
#include <string.h>

#include "Strings.h"

struct char_struct {
    lisp_object obj;
    char val[2];
};

char *CharToString(lisp_object *obj) {
    Char *ch = (Char*)obj;
    return ch->val;
}

LLVMValueRef Char_codegen(lisp_object *obj) {
    return NULL;
}

Char *NewChar(char ch) {
    Char *ret =malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = CHAR_TYPE;
    ret->obj.codegen = &Char_codegen;
    ret->obj.toString = &CharToString;

    ret->val[0] = ch;
    ret->val[1] = '\0';

    return ret;
}

struct string_struct {
    lisp_object obj;
    char *str;
};

char *StringToString(lisp_object *obj) {
    String *str = (String*)obj;
    return str->str;
}

LLVMValueRef String_codegen(lisp_object *obj) {
    return NULL;
}

String *NewString(char *str) {
    String *ret =malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = STRING_TYPE;
    ret->obj.codegen = &String_codegen;
    ret->obj.toString = &StringToString;

    ret->str = str;

    return ret;
}
