#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "llvm-c/Types.h"
#include "llvm-c/Core.h"

#include "Numbers.h"

struct Integer_struct {
    lisp_object obj;
    long val;
    char *str;
};

static LLVMValueRef Integer_codegen(const lisp_object *obj) {
    assert(obj->type == INTEGER_type);
    Integer *int_obj = (Integer*)obj;
    return LLVMConstInt(LLVMInt64Type(), (unsigned long) int_obj->val, (LLVMBool)1);
}

static const char *IntegerToString(const lisp_object *obj) {
    assert(obj->type == INTEGER_type);
    Integer *IntObj = (Integer*)obj;
    if(IntObj->str == NULL) {
        int size = snprintf(NULL, 0, "%ld", IntObj->val);
        char *str = malloc((size + 1) * sizeof(*str));
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
    Integer *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = INTEGER_type;
    ret->obj.codegen = Integer_codegen;
	ret->obj.toString = IntegerToString;
    ret->obj.copy = IntegerCopy;
	ret->obj.fns = &NullInterface;

    ret->val = i;

    return ret;
}

long IntegerValue(Integer *I) {
    return I->val;
}

struct Float_struct {
    lisp_object obj;
    double val;
    char *str;
};

static LLVMValueRef Float_codegen(const lisp_object *obj) {
    assert(obj->type == FLOAT_type);
    Float *flt_obj = (Float*)obj;
    return LLVMConstReal(LLVMDoubleType(), flt_obj->val);
}

static const char *FloatToString(const lisp_object *obj) {
    assert(obj->type == FLOAT_type);
    Float *FltObj = (Float*)obj;
    if(FltObj->str == NULL) {
        int size = snprintf(NULL,0, "%g", FltObj->val);
        char *str = malloc((size + 1) * sizeof(*str));
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
    Float *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = FLOAT_type;
    ret->obj.codegen = Float_codegen;
	ret->obj.toString = FloatToString;
    ret->obj.copy = FloatCopy;
	ret->obj.fns = &NullInterface;

    ret->val = x;

    return ret;
}

double FloatValue(Float *X) {
    return X->val;
}
