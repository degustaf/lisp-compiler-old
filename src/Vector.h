#ifndef VECTOR_H
#define VECTOR_H

#include "LispObject.h"

typedef struct Vector_struct Vector;

const Vector *CreateVector(size_t count, lisp_object **entries);

const Vector *const EmptyVector;

#endif /* VECTOR_H */
