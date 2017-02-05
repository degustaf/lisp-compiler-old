#include <stdlib.h>
#include <string.h>

#include "llvm-c/Types.h"
#include "llvm-c/Core.h"

#include "Numbers.h"

struct Integer_struct {
    lisp_object obj;
    long val;
};

LLVMValueRef Integer_codegen(lisp_object *obj) {
    Integer *int_obj = (Integer*)obj;
    return LLVMConstInt(LLVMInt64Type(), (unsigned long) int_obj->val, (LLVMBool)1);
}

Integer *NewInteger(long i) {
    Integer *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = INTEGER_type;
    ret->obj.codegen = &Integer_codegen;

    ret->val = i;

    return ret;
}

long IntegerValue(Integer *I) {
    return I->val;
}

struct Float_struct {
    lisp_object obj;
    double val;
};

LLVMValueRef Float_codegen(lisp_object *obj) {
    Float *flt_obj = (Float*)obj;
    return LLVMConstReal(LLVMDoubleType(), flt_obj->val);
}

Float *NewFloat(double x) {
    Float *ret = malloc(sizeof(*ret));
    memset(ret, 0, sizeof(*ret));

    ret->obj.type = FLOAT_type;
    ret->obj.codegen = &Float_codegen;

    ret->val = x;

    return ret;
}

double FloatValue(Float *X) {
    return X->val;
}
