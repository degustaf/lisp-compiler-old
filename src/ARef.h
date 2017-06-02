#ifndef AREF_H
#define AREF_H

#include "LispObject.h"
#include <stdbool.h>

void Validate(bool (*validator)(const lisp_object*), const lisp_object *val);

#endif /* AREF_H */
