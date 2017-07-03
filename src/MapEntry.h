#ifndef MAPENTRY_H
#define MAPENTRY_H

#include "LispObject.h"

typedef struct {
	lisp_object obj;
	const lisp_object *key;
	const lisp_object *val;
} MapEntry;

const MapEntry* NewMapEntry(const lisp_object *key, const lisp_object *val);

#endif /* MAPENTRY_H */
