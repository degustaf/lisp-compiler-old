#ifndef AFN_H
#define AFN_H

#include "LispObject.h"
#include "Interfaces.h"

const lisp_object* invoke0AFn(const IFn *self);
const lisp_object* invoke1AFn(const IFn *self, const lisp_object*);
const lisp_object* invoke2AFn(const IFn *self, const lisp_object*, const lisp_object*);
const lisp_object* invoke3AFn(const IFn *self, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* invoke4AFn(const IFn *self, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* invoke5AFn(const IFn *self, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* applyToAFn(const IFn *self, const ISeq*);

#endif /* AFN_H */
