#ifndef COMPILER_H
#define COMPILER_H

#include "LispObject.h"

#include <stdio.h>

const lisp_object* Eval(const lisp_object *form);
const lisp_object* compilerLoad(FILE *reader, const char *path, const char *name);

#endif /* COMPILER_H */
