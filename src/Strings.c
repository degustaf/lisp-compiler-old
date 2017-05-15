#include "Strings.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct char_struct {
    lisp_object obj;
    char val[2];
};

static const char *CharToString(const lisp_object *obj) {
	assert(obj->type == CHAR_type);
    Char *ch = (Char*)obj;
    return ch->val;
}

// LLVMValueRef Char_codegen(lisp_object *obj) {
//     return NULL;
// }

Char *NewChar(char ch) {
    Char *ret =malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = CHAR_type;
    // ret->obj.codegen = Char_codegen;
    ret->obj.toString = CharToString;
	ret->obj.fns = &NullInterface;

    ret->val[0] = ch;
    ret->val[1] = '\0';

    return ret;
}

struct string_struct {
    lisp_object obj;
    char *str;
};

static const char *StringToString(const lisp_object *obj) {
	assert(obj->type == STRING_type);
    String *str = (String*)obj;
    return str->str;
}

// LLVMValueRef String_codegen(lisp_object *obj) {
//     return NULL;
// }

String *NewString(char *str) {
	// fprintf(stderr, "In NewString: str = '%s'\n", str);
    String *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = STRING_type;
    // ret->obj.codegen = String_codegen;
    ret->obj.toString = StringToString;
	ret->obj.fns = &NullInterface;

    ret->str = str;

    return ret;
}
