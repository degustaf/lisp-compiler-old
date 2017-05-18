#include "Strings.h"

#include <assert.h>
#include <string.h>

#include "gc.h"

struct char_struct {
    lisp_object obj;
    char val[2];
};

static const char *CharToString(const lisp_object *obj) {
	assert(obj->type == CHAR_type);
    Char *ch = (Char*)obj;
    return ch->val;
}

static const lisp_object *CharCopy(const lisp_object *obj) {
    assert(obj->type == CHAR_type);
    Char *ch = (Char*)obj;
    return (lisp_object*)NewChar(ch->val[0]);
}

Char *NewChar(char ch) {
    Char *ret = GC_MALLOC(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = CHAR_type;
    // ret->obj.codegen = Char_codegen;
    ret->obj.toString = CharToString;
    ret->obj.copy = CharCopy;
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

static const lisp_object *StringCopy(const lisp_object *obj) {
    assert(obj->type == STRING_type);
    String *str = (String*)obj;
    return (lisp_object*)NewString(str->str);
}

String *NewString(char *str) {
    String *ret = GC_MALLOC(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = STRING_type;
    ret->obj.toString = StringToString;
    ret->obj.copy = StringCopy;
	ret->obj.fns = &NullInterface;

	size_t len = strlen(str);
	ret->str = GC_MALLOC_ATOMIC((len+1) * sizeof(*str));
    strncpy(ret->str, str, len);
	ret->str[len] = '\0';

    return ret;
}
