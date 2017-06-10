#include "Var.h"

#include "ARef.h"
#include "Bool.h"
#include "gc.h"
#include "Interfaces.h"
#include "Keyword.h"
#include "Map.h"
#include "pthread.h"
#include "StringWriter.h"
#include "Util.h"

// Support functions

// Unbound

typedef struct {
	lisp_object obj;
	const Var* v;
} Unbound;

// Unbound function declarations.

static const Unbound *NewUnbound(const Var *v);
static const char *toStringUnbound(const lisp_object *obj);

IFn_vtable Unbound_IFn_vtable = {
	NULL,	// invoke0	// TODO
	NULL,		// invoke1	// TODO
	NULL,		// invoke2	// TODO
	NULL,	// invoke3	// TODO
	NULL,	// invoke4	// TODO
	NULL,	// invoke5	// TODO
	NULL,		// applyTo	// TODO
};

interfaces Unbound_interfaces = {
	NULL,					// SeqableFns
	NULL,					// ReversibleFns
	NULL,					// ICollectionFns
	NULL,					// IStackFns
	NULL,					// ISeqFns
	&Unbound_IFn_vtable,	// IFnFns
	NULL,					// IVectorFns
	NULL,					// IMapFns
};

// Frame

typedef struct Frame_struct {
	const IMap *bindings;
	const struct Frame_struct *prev;
} Frame;

// Frame function declarations.

static const Frame *NewFrame(const IMap *bindings, const Frame *prev);
// static const Frame *cloneFrame(Frame *f);

static const Frame _TopFrame = {(const IMap*)&_EmptyHashMap, NULL};
static const Frame *const TopFrame = &_TopFrame;
static __thread const Frame *dval = &_TopFrame;

// Var

struct Var_struct{
	lisp_object obj;
	const lisp_object *root;
	bool dynamic;
	bool threadBound;
	const Symbol *sym;
	const Namespace *ns;
	bool (*validator)(const lisp_object*);
};

// Var function declarations.

static void bindRoot(Var *v, const lisp_object *obj);
static bool hasRoot(const Var *v);
static const IFn* fn(const Var *v);
static const lisp_object* invoke0Var(const IFn*);
static const lisp_object* invoke1Var(const IFn *self, const lisp_object *arg1);
static const lisp_object* invoke2Var(const IFn*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke3Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke4Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke5Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* applyToVar(const IFn*, const ISeq*);

IFn_vtable Var_IFn_vtable = {
	invoke0Var,	// invoke0
	invoke1Var,	// invoke1
	invoke2Var,	// invoke2
	invoke3Var,	// invoke3
	invoke4Var,	// invoke4
	invoke5Var,	// invoke5
	applyToVar,	// applyTo
};

interfaces Var_interfaces = {
	NULL,				// SeqableFns
	NULL,				// ReversibleFns
	NULL,				// ICollectionFns
	NULL,				// IStackFns
	NULL,				// ISeqFns
	&Var_IFn_vtable,	// IFnFns
	NULL,				// IVectorFns
	NULL,				// IMapFns
};

// Unbound Function Definitions.

static const Unbound *NewUnbound(const Var *v) {
	Unbound *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = UNBOUND_type;
	ret->obj.toString = toStringUnbound;
	ret->obj.copy = NULL;	// TODO
	ret->obj.fns = &Unbound_interfaces;

	ret->v = v;
	return ret;
}

static const char *toStringUnbound(const lisp_object *obj) {
	assert(obj->type == UNBOUND_type);
	const Unbound *u = (const Unbound*) obj;

	StringWriter *sw = NewStringWriter();
	AddString(sw, "Unbound: ");
	AddString(sw, toString((const lisp_object*) u->v));

	return WriteString(sw);
}

// Frame Function Definitions.

static const Frame *NewFrame(const IMap *bindings, const Frame *prev) {
	Frame *ret = GC_MALLOC(sizeof(*ret));

	ret->bindings = bindings;
	ret->prev = prev;

	return ret;
}

// static const Frame *cloneFrame(Frame *f);	// TODO

// Var Function Definitions.

Var* NewVar(const Namespace *ns, const Symbol *sym, const lisp_object *root) {
	Var *ret = GC_MALLOC(sizeof(*ret));

	ret->obj.type = VAR_type;
	ret->obj.toString = NULL;	// TODO
	ret->obj.copy = NULL;		// TODO
	ret->obj.meta = (IMap*) EmptyHashMap;
	ret->obj.fns = &Var_interfaces;

	ret->ns = ns;
	ret->sym = sym;
	ret->dynamic = false;
	ret->threadBound = false;
	ret->validator = NULL;
	if(root) {
		ret->root = root;
	} else {
		ret->root = (const lisp_object*) NewUnbound(ret);
	}

	return ret;
}

Var* internVar(Namespace *ns, const Symbol *sym, const lisp_object* root, bool replaceRoot) {
	Var *ret = internNS(ns, sym);
	if(!hasRoot(ret) || replaceRoot) {
		bindRoot(ret, root);
	}
	return ret;
}

Var* createVar(const lisp_object* root) {
	return NewVar(NULL, NULL, root);
}

const lisp_object *getTag(const Var *v) {
	const IMap *m = v->obj.meta;
	return m->obj.fns->IMapFns->entryAt(m, (lisp_object*)tagKW);
}

const lisp_object *getVar(const Var *v) {
	if(v->threadBound)
		return v->root;
	return deref(v);
}

void setTag(Var *v, const Symbol *tag) {
	const IMap *m = v->obj.meta;
	v->obj.meta = m->obj.fns->IMapFns->assoc(m, (lisp_object*)tagKW, (lisp_object*)tag);
}

void setMeta(Var *v, const IMap *m) {
	v->obj.meta = m;
}

void setMacro(Var *v) {
	const IMap *m = v->obj.meta;
	v->obj.meta = m->obj.fns->IMapFns->assoc(m, (lisp_object*)macroKW, (lisp_object*)True);
}

void setVar(Var *v, const lisp_object *val) {
	Validate(v->validator, val);
	const IMap *m = dval->bindings;
	dval = NewFrame(m->obj.fns->IMapFns->assoc(m, (lisp_object*)v, val), dval->prev);
}

bool isMacroVar(Var *v) {
	const IMap *m = v->obj.meta;
	return boolCast(m->obj.fns->IMapFns->entryAt(m, (lisp_object*)macroKW));
}

Var *setDynamic(Var *v) {
	v->dynamic = true;
	return v;
}

Var *setDynamic1(Var *v, bool b) {
	v->dynamic = b;
	return v;
}

bool isDynamic(const Var *v) {
	return v->dynamic;
}

bool isBound(const Var *v) {
	const IMap *bmap = dval->bindings;
	return hasRoot(v) || (v->threadBound && bmap->obj.fns->IMapFns->entryAt(bmap, (lisp_object*)v));
}

bool isPublic(const Var *v) {
	const IMap *m = v->obj.meta;
	return m->obj.fns->IMapFns->entryAt(m, (lisp_object*)v);
}

const lisp_object* deref(const Var *v) {
	return v->root;
}

const Namespace *getNamespaceVar(const Var *v) {
	return v->ns;
}

void pushThreadBindings(const IMap *bindings) {
	const Frame *f = dval;
	const IMap *bmap = dval->bindings;
	for(const ISeq *s = bindings->obj.fns->SeqableFns->seq((const Seqable*)bindings); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *e = (const MapEntry*)s->obj.fns->ISeqFns->first(s);
		Var *v = (Var*)e->key;
		if(!v->dynamic) {
			// printf("Can't dynamically bind non-dynamic var: %s/%s", v->ns, v->sym)
		}
		Validate(v->validator, e->val);
		v->threadBound = true;
		bmap = bmap->obj.fns->IMapFns->assoc(bmap, (const lisp_object*)v, e->val);
	}
	dval = NewFrame(bmap, f);
}

void popThreadBindings(void) {
	const Frame *f = dval->prev;
	if(f == NULL) {
		NULL;	// TODO throw new IllegalStateException("Pop without matching push");
	}
	dval = f;
}

static void bindRoot(Var *v, const lisp_object *obj) {
	Validate(v->validator, obj);
	v->root = obj;
	// TODO Handle meta.
	// TODO Handle Watches.
}

static bool hasRoot(const Var *v) {
	return v->root->type != UNBOUND_type;
}

static const IFn* fn(const Var *v) {
	assert(isIFn(v->root));
	return (IFn*) v->root;
}

static const lisp_object* invoke0Var(const IFn *self) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke0(f);
}

static const lisp_object* invoke1Var(const IFn *self, const lisp_object *arg1) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke1(f, arg1);
}

static const lisp_object* invoke2Var(const IFn *self, const lisp_object *arg1, const lisp_object *arg2) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke2(f, arg1, arg2);
}

static const lisp_object* invoke3Var(const IFn *self, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke3(f, arg1, arg2, arg3);
}

static const lisp_object* invoke4Var(const IFn *self, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke4(f, arg1, arg2, arg3, arg4);
}

static const lisp_object* invoke5Var(const IFn *self, const lisp_object *arg1, const lisp_object *arg2, const lisp_object *arg3, const lisp_object *arg4, const lisp_object *arg5) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->invoke5(f, arg1, arg2, arg3, arg4, arg5);
}

static const lisp_object* applyToVar(const IFn *self, const ISeq *args) {
	assert(self->obj.type == VAR_type);
	const Var *v = (Var*)self;
	const IFn *f = fn(v);
	return f->obj.fns->IFnFns->applyTo(f, args);
}
