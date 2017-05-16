#ifndef MAP_H
#define MAP_H

#include "LispObject.h"

typedef struct {
	const lisp_object *key;
	const lisp_object *val;
} MapEntry;

typedef struct HashMap_struct HashMap;

const HashMap *CreateHashMap(size_t count, const lisp_object **entries);

const HashMap *const EmptyHashMap;

#endif /* MAP_H */
