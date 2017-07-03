#include "RestFn.h"

#include "Error.h"
#include "Interfaces.h"

struct RestFn_struct {
	BASE_RESTFN
};

bool isRestFn(__attribute__((unused)) const IFn *f) {
	return false;	// TODO
}

const lisp_object* invoke0RestFn(const IFn *f) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->getRequiredArity()) {
		case 0:
			return fn->doInvoke0(NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke1RestFn(const IFn *f, const lisp_object *arg1) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->getRequiredArity()) {
		case 0:
			return fn->doInvoke0(arg1);
		case 1:
			return fn->doInvoke1(arg1, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke2RestFn(const IFn*, const lisp_object*, const lisp_object*);
const lisp_object* invoke3RestFn(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* invoke4RestFn(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* invoke5RestFn(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
const lisp_object* applyToRestFn(const IFn*, const ISeq*);

const IFn_vtable RestFnIFn_vtable = {
	invoke0RestFn,	// invoke0
	invoke1RestFn,	// invoke1
	invoke2RestFn,	// invoke2
	invoke3RestFn,	// invoke3
	invoke4RestFn,	// invoke4
	invoke5RestFn,	// invoke5
	applyToRestFn,	// applyTo
};

const interfaces _RestFnInterfaces = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&RestFnIFn_vtable,
	NULL,
	NULL,
};

const interfaces *const RestFnInterfaces = &_RestFnInterfaces;
