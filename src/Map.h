#ifndef MAP_H
#define MAP_H

#include "LispObject.h"

typedef struct HashMap_struct HashMap;

const HashMap *CreateHashMap(size_t count, const lisp_object **entries);

extern const HashMap _EmptyHashMap;
const HashMap *const EmptyHashMap;

#endif /* MAP_H */
