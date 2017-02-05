#ifndef NUMBERS_H
#define NUMBERS_H

#include "LispObject.h"

typedef struct Integer_struct Integer;
typedef struct Float_struct Float;

// Integer Functions
Integer *NewInteger(long i);
long IntegerValue(Integer *I);

// Float Functions
Float *NewFloat(double x);
double FloatValue(Float *X);

#endif /* NUMBERS_H */
