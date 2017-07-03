#ifndef AVECTOR_H
#define AVECTOR_H

#include "LispObject.h"

#include "Interfaces.h"
#include "MapEntry.h"

const ISeq* rseq(const Seqable *s);
const ICollection* emptyAVector(void);
bool EquivAVector(const ICollection*, const lisp_object*);
bool EqualsAVector(const struct lisp_object_struct *x, const struct lisp_object_struct *y);
const lisp_object* peekAVector(const IStack*);
const lisp_object* invoke1AVector(const IFn*, const lisp_object*);
size_t lengthAVector(const IVector*);
const IVector* assocAVector(const IVector*, const lisp_object*, const lisp_object*);
const MapEntry* entryAtAVector(const IVector*, const lisp_object*);

#endif /* AVECTOR_H */
