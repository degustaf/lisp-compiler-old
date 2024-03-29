#include "AFn.h"

#include "Error.h"
#include "Util.h"

const lisp_object* invoke0AFn(const IFn *self) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* invoke1AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* invoke2AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* invoke3AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* invoke4AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3, __attribute__((unused)) const lisp_object *arg4) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* invoke5AFn(const IFn *self, __attribute__((unused)) const lisp_object *arg1, __attribute__((unused)) const lisp_object *arg2, __attribute__((unused)) const lisp_object *arg3, __attribute__((unused)) const lisp_object *arg4, __attribute__((unused)) const lisp_object *arg5) {
	NewArityError(0, object_type_string[self->obj.type]);
	__builtin_unreachable();
}

const lisp_object* applyToAFn(const IFn *self, const ISeq *args) {
	size_t arg_count = args->obj.fns->ICollectionFns->count((ICollection*)args);
	const lisp_object *arg1 = first((lisp_object*)args);
	args = next((lisp_object*)args);
	const lisp_object *arg2 = first((lisp_object*)args);
	args = next((lisp_object*)args);
	const lisp_object *arg3 = first((lisp_object*)args);
	args = next((lisp_object*)args);
	const lisp_object *arg4 = first((lisp_object*)args);
	args = next((lisp_object*)args);
	const lisp_object *arg5 = first((lisp_object*)args);
	args = next((lisp_object*)args);
	switch(arg_count) {
		case 0:
			return self->obj.fns->IFnFns->invoke0(self);
		case 1:
			return self->obj.fns->IFnFns->invoke1(self, arg1);
		case 2:
			return self->obj.fns->IFnFns->invoke2(self, arg1, arg2);
		case 3:
			return self->obj.fns->IFnFns->invoke3(self, arg1, arg2, arg3);
		case 4:
			return self->obj.fns->IFnFns->invoke4(self, arg1, arg2, arg3, arg4);
		case 5:
			return self->obj.fns->IFnFns->invoke5(self, arg1, arg2, arg3, arg4, arg5);
		default: {
			exception e = {Exception, "applyTo needs to be extended."};
			Raise(e);
		}
	}
	__builtin_unreachable();
}
