#ifndef VECTOR_H
#define VECTOR_H

#include "LispObject.h"

typedef struct Vector_struct Vector;

const Vector *CreateVector(size_t count, lisp_object const **entries);

const Vector *const EmptyVector;

#include "Interfaces.h"

int indexOf(const IVector *iv, const lisp_object *o);

#endif /* VECTOR_H */
