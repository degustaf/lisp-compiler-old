#ifndef LISP_UTIL_H
#define LISP_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "Interfaces.h"
#include "LispObject.h"
#include "StringWriter.h"

#define HASH_MIXER 0x9e3779b9

bool EqualBase(const lisp_object *x, const lisp_object *y);
bool Equals(const lisp_object *x, const lisp_object *y);
bool Equiv(const lisp_object *x, const lisp_object *y);

uint32_t HashEq(const lisp_object *x);

uint32_t hashCombine(uint32_t x, uint32_t y);

const char *toString(const lisp_object *obj);

const lisp_object *get(const lisp_object *coll, const lisp_object *key, const lisp_object *NotFound);

const ISeq *seq(const lisp_object *obj);

bool boolCast(const lisp_object *obj);

#endif /* LISP_UTIL_H */
