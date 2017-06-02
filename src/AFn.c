#include "AFn.h"

#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.

const lisp_object* invoke0AFn(const IFn *self) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

const lisp_object* invoke1AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

const lisp_object* invoke2AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

const lisp_object* invoke3AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

const lisp_object* invoke4AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3, __attribute__((unused)) const lisp_object *arg4) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

const lisp_object* invoke5AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3, __attribute__((unused)) const lisp_object *arg4, __attribute__((unused)) const lisp_object *arg5) {
	return (const lisp_object*) NewArityError(0, object_type_string[self->obj.type]);
}

