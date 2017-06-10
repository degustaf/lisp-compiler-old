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
const lisp_object *first(const lisp_object *x);
const lisp_object *second(const lisp_object *x);
const ISeq *next(const lisp_object *x);
const ISeq* cons(const lisp_object *x, const lisp_object *coll);
const ICollection* conj_(const ICollection *coll, const lisp_object *x);
const lisp_object* assoc(const lisp_object *coll, const lisp_object *key, const lisp_object *val);
bool boolCast(const lisp_object *obj);
const lisp_object *withMeta(const lisp_object *obj, const IMap *meta);
size_t count(const lisp_object *obj);
const ISeq* listStar1(const lisp_object *arg1, const ISeq *rest);
const ISeq* listStar2(const lisp_object *arg1, const lisp_object *arg2, const ISeq *rest);
const ISeq* listStar3(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const ISeq *rest);
const ISeq* listStar4(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const ISeq *rest);
const ISeq* listStar5(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const lisp_object *arg5, const ISeq *rest);

#endif /* LISP_UTIL_H */
