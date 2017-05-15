#ifndef LISP_UTIL_H
#define LISP_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "LispObject.h"
#include "StringWriter.h"

bool Equiv(const lisp_object *x, const lisp_object *y);

uint32_t HashEq(const lisp_object *x);

const char *toString(const lisp_object *obj);

#endif /* LISP_UTIL_H */
