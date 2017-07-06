#ifndef RESTFN_H
#define RESTFN_H

#include "LispObject.h"

typedef struct RestFn_struct RestFn;

#define BASE_RESTFN \
	lisp_object obj; \
	size_t RequiredArity; \
	const lisp_object* (*doInvoke0)(const lisp_object *args); \
	const lisp_object* (*doInvoke1)(const lisp_object *arg1, const lisp_object *args); \
	const lisp_object* (*doInvoke2)(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *args); \
	const lisp_object* (*doInvoke3)(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *args); \
	const lisp_object* (*doInvoke4)(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const lisp_object *args); \
	const lisp_object* (*doInvoke5)(const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const lisp_object *arg5, const lisp_object *args);

extern const interfaces *const RestFnInterfaces;
extern const RestFn *const creator;

#endif /* RESTFN_H */
