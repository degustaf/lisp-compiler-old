#ifndef LIST_H
#define LIST_H

#include "LispObject.h"

typedef struct List_struct List;

const List *NewList(const lisp_object *const first);
const List *CreateList(lisp_object **entries, size_t count);

const List *const EmptyList;

#endif /* LIST_H */
