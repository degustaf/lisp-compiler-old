#ifndef CONS_H
#define CONS_H

#include "LispObject.h"
#include "Interfaces.h"

typedef struct Cons_struct Cons;

const Cons *NewCons(const lisp_object *obj, const ISeq *s);

#endif /* CONS_H */
