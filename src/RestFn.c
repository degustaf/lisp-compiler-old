#include "RestFn.h"

#include "AFn.h"
#include "Error.h"
#include "Interfaces.h"
#include "List.h"
#include "Map.h"
#include "Util.h"

struct RestFn_struct {
	BASE_RESTFN
};

bool isRestFn(const IFn *f) {
	return f->obj.type == RESTFN_type;
}

const lisp_object* invoke0RestFn(const IFn *f) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0:
			return fn->doInvoke0(NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke1RestFn(const IFn *f, const lisp_object *arg1) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0: {
			const lisp_object *margs[] = {arg1};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke0((lisp_object*)CreateList(margc, margs));
		}
		case 1:
			return fn->doInvoke1(arg1, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke2RestFn(const IFn *f, const lisp_object *arg1, const lisp_object *arg2) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0: {
			const lisp_object *margs[] = {arg1, arg2};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke0((lisp_object*)CreateList(margc, margs));
		}
		case 1: {
			const lisp_object *margs[] = {arg2};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke1(arg1, (lisp_object*)CreateList(margc, margs));
		}
		case 2:
			return fn->doInvoke2(arg1, arg2, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke3RestFn(const IFn *f, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0: {
			const lisp_object *margs[] = {arg1, arg2, arg3};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke0((lisp_object*)CreateList(margc, margs));
		}
		case 1: {
			const lisp_object *margs[] = {arg2, arg3};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke1(arg1, (lisp_object*)CreateList(margc, margs));
		}
		case 2: {
			const lisp_object *margs[] = {arg3};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke2(arg1, arg2, (lisp_object*)CreateList(margc, margs));
		}
		case 3:
			return fn->doInvoke3(arg1, arg2, arg3, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke4RestFn(const IFn *f, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0: {
			const lisp_object *margs[] = {arg1, arg2, arg3, arg4};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke0((lisp_object*)CreateList(margc, margs));
		}
		case 1: {
			const lisp_object *margs[] = {arg2, arg3, arg4};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke1(arg1, (lisp_object*)CreateList(margc, margs));
		}
		case 2: {
			const lisp_object *margs[] = {arg3, arg4};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke2(arg1, arg2, (lisp_object*)CreateList(margc, margs));
		}
		case 3: {
			const lisp_object *margs[] = {arg4};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke3(arg1, arg2, arg3, (lisp_object*)CreateList(margc, margs));
		}
		case 4:
			return fn->doInvoke4(arg1, arg2, arg3, arg4, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* invoke5RestFn(const IFn *f, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const lisp_object *arg5) {
	assert(isRestFn(f));
	const RestFn *fn = (RestFn*)f;
	switch(fn->RequiredArity) {
		case 0: {
			const lisp_object *margs[] = {arg1, arg2, arg3, arg4, arg5};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke0((lisp_object*)CreateList(margc, margs));
		}
		case 1: {
			const lisp_object *margs[] = {arg2, arg3, arg4, arg5};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke1(arg1, (lisp_object*)CreateList(margc, margs));
		}
		case 2: {
			const lisp_object *margs[] = {arg3, arg4, arg5};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke2(arg1, arg2, (lisp_object*)CreateList(margc, margs));
		}
		case 3: {
			const lisp_object *margs[] = {arg4, arg5};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke3(arg1, arg2, arg3, (lisp_object*)CreateList(margc, margs));
		}
		case 4: {
			const lisp_object *margs[] = {arg5};
			size_t margc = sizeof(margs)/sizeof(margs[0]);
			return fn->doInvoke4(arg1, arg2, arg3, arg4, (lisp_object*)CreateList(margc, margs));
		}
		case 5:
			return fn->doInvoke5(arg1, arg2, arg3, arg4, arg5, NULL);
		default:
			return NewArityError(0, fn->obj.toString((lisp_object*)fn));
	}
}

const lisp_object* applyToRestFn(const IFn*, const ISeq*);

const IFn_vtable RestFnIFn_vtable = {
	invoke0RestFn,	// invoke0
	invoke1RestFn,	// invoke1
	invoke2RestFn,	// invoke2
	invoke3RestFn,	// invoke3
	invoke4RestFn,	// invoke4
	invoke5RestFn,	// invoke5
	applyToAFn,		// applyTo
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

const lisp_object* doInvoke0Creator(const lisp_object *args) {
	const ISeq *s = seq(args);
	size_t margc = s->obj.fns->ICollectionFns->count((ICollection*)s);
	const lisp_object *margs[margc];

	for(size_t i = 0; i < margc; i++) {
		margs[i] = s->obj.fns->ISeqFns->first(s);
		s = s->obj.fns->ISeqFns->next(s);
	}

	return (lisp_object*) CreateList(margc, margs);
}

const RestFn _creator = {
	{
		RESTFN_type,			// type
		sizeof(_creator),		// size
		NULL,					// toString
		NULL,					// Equals
		(IMap*) &_EmptyHashMap,	// meta
		&_RestFnInterfaces		// fns
	},							// obj
	0,							// RequiredArity
	doInvoke0Creator,			// doInvoke0
	NULL,						// doInvoke1
	NULL,						// doInvoke2
	NULL,						// doInvoke3
	NULL,						// doInvoke4
	NULL,						// doInvoke5
};

const RestFn *const creator = &_creator;
