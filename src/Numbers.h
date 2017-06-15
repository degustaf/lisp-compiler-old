#ifndef NUMBERS_H
#define NUMBERS_H

#include "LispObject.h"

typedef struct Integer_struct Integer;
typedef struct Float_struct Float;

// Integer Functions
Integer *NewInteger(long i);
long IntegerValue(const Integer *I);

// Float Functions
Float *NewFloat(double x);
double FloatValue(const Float *X);

typedef struct {	// Number
    lisp_object obj;
} Number;

static inline bool isNumber(const lisp_object *obj) {
	return obj->type == INTEGER_type || obj->type == FLOAT_type;
}

#endif /* NUMBERS_H */
