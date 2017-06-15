#include "Numbers.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"

struct Integer_struct {
    lisp_object obj;
    long val;
    char *str;
};

static const char *IntegerToString(const lisp_object *obj) {
    assert(obj->type == INTEGER_type);
    Integer *IntObj = (Integer*)obj;
    if(IntObj->str == NULL) {
        int size = snprintf(NULL, 0, "%ld", IntObj->val);
        char *str = GC_MALLOC_ATOMIC((size + 1) * sizeof(*str));
        snprintf(str, size+1, "%ld", IntObj->val);
        IntObj->str = str;
    }

    return IntObj->str;
}

static const lisp_object *IntegerCopy(const lisp_object *obj) {
    assert(obj->type == INTEGER_type);
    Integer *IntObj = (Integer*)obj;
    return (lisp_object*)NewInteger(IntObj->val);
}

Integer *NewInteger(long i) {
    Integer *ret = GC_MALLOC(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = INTEGER_type;
	ret->obj.toString = IntegerToString;
    ret->obj.copy = IntegerCopy;
	ret->obj.fns = &NullInterface;

    ret->val = i;

    return ret;
}

long IntegerValue(const Integer *I) {
    return I->val;
}

struct Float_struct {
    lisp_object obj;
    double val;
    char *str;
};

static const char *FloatToString(const lisp_object *obj) {
    assert(obj->type == FLOAT_type);
    Float *FltObj = (Float*)obj;
    if(FltObj->str == NULL) {
        int size = snprintf(NULL,0, "%g", FltObj->val);
        char *str = GC_MALLOC_ATOMIC((size + 1) * sizeof(*str));
        snprintf(str,size+1, "%g", FltObj->val);
        FltObj->str = str;
    }

    return FltObj->str;
}

static const lisp_object *FloatCopy(const lisp_object *obj) {
    assert(obj->type == FLOAT_type);
    Float *FltObj = (Float*)obj;
    return (lisp_object*)NewFloat(FltObj->val);
}

Float *NewFloat(double x) {
    Float *ret = GC_MALLOC(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = FLOAT_type;
	ret->obj.toString = FloatToString;
    ret->obj.copy = FloatCopy;
	ret->obj.fns = &NullInterface;

    ret->val = x;

    return ret;
}

double FloatValue(const Float *X) {
    return X->val;
}
