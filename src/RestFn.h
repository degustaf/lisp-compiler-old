#ifndef RESTFN_H
#define RESTFN_H

#include "LispObject.h"

typedef struct RestFn_struct RestFn;

#define BASE_RESTFN \
	lisp_object obj; \
	size_t (*getRequiredArity)(void); \
	const lisp_object* (*doInvoke0)(const lisp_object *args);

extern const interfaces *const RestFnInterfaces;

#endif /* RESTFN_H */
