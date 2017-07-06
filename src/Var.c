#include "Var.h"

#include <stdio.h>	// For Debugging.

#include "AFn.h"
#include "ARef.h"
#include "Bool.h"
#include "Error.h"
#include "gc.h"
#include "Interfaces.h"
#include "Keyword.h"
#include "Map.h"
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

const IFn_vtable Unbound_IFn_vtable = {
	invoke0AFn,
	invoke1AFn,
	invoke2AFn,
	invoke3AFn,
	invoke4AFn,
	invoke5AFn,
	applyToAFn,
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

static bool hasRoot(const Var *v);
static const IFn* fn(const Var *v);
static const lisp_object* invoke0Var(const IFn*);
static const lisp_object* invoke1Var(const IFn *self, const lisp_object *arg1);
static const lisp_object* invoke2Var(const IFn*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke3Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke4Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* invoke5Var(const IFn*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*, const lisp_object*);
static const lisp_object* applyToVar(const IFn*, const ISeq*);
static const char* toStringVar(const lisp_object *obj);

const IFn_vtable Var_IFn_vtable = {
	invoke0Var,
	invoke1Var,
	invoke2Var,
	invoke3Var,
	invoke4Var,
	invoke5Var,
	applyToVar,
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
	ret->obj.toString = toStringVar;
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
	const MapEntry *me = m->obj.fns->IMapFns->entryAt(m, (lisp_object*)tagKW);
	return me ? me->val : NULL;
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
	printf("In setVar\n");
	Validate(v->validator, val);
	const IMap *m = dval->bindings;
	dval = NewFrame(m->obj.fns->IMapFns->assoc(m, (lisp_object*)v, val), dval->prev);
	printf("dval = %p\n", (void*)dval);
	if(dval)
		printf("dval->bindings = %p\n", (void*)dval->bindings);
	fflush(stdout);
}

bool isMacroVar(Var *v) {
	return boolCast(get((lisp_object*)v->obj.meta, (lisp_object*)macroKW, NULL));
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
	printf("In isBound.\n");
	printf("TopFrame = %p\n", (void*)TopFrame);
	printf("v = %p\n", (void*)v);
	printf("dval = %p\n", (void*)dval);
	if(dval)
		printf("dval->bindings = %p\n", (void*)dval->bindings);
	fflush(stdout);
	const IMap *bmap = dval->bindings;
	printf("hasRoot is %d\n", hasRoot(v));
	printf("v->threadBound = %p\n", (void*)v->threadBound);
	printf("bmap->obj = %p\n", (void*)&(bmap->obj));
	printf("bmap->obj.fns = %p\n", (void*)bmap->obj.fns);
	printf("bmap->obj.type = %d\n", bmap->obj.type);
	fflush(stdout);
	printf("bmap->obj.type = %s\n", object_type_string[bmap->obj.type]);
	fflush(stdout);
	return hasRoot(v) || (v->threadBound && bmap->obj.fns->IMapFns->entryAt(bmap, (lisp_object*)v));
}

bool isPublic(const Var *v) {
	return boolCast(get((lisp_object*)v->obj.meta, (lisp_object*)privateKW, NULL));
}

const lisp_object* deref(const Var *v) {
	return v->root;
}

void bindRoot(Var *v, const lisp_object *obj) {
	Validate(v->validator, obj);
	v->root = obj;
	// TODO Handle meta.
	// TODO Handle Watches.
}

const Namespace *getNamespaceVar(const Var *v) {
	return v->ns;
}

const Symbol *getSymbolVar(const Var *v) {
	return v->sym;
}

void pushThreadBindings(const IMap *bindings) {
	printf("In pushThreadBindings.\n");
	const Frame *f = dval;
	const IMap *bmap = dval->bindings;
	printf("bmap = %p\n", (void*)bmap);
	if(bmap)
		printf("bmap->type = %d\n", bmap->obj.type);
	fflush(stdout);
	for(const ISeq *s = bindings->obj.fns->SeqableFns->seq((const Seqable*)bindings); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *e = (const MapEntry*)s->obj.fns->ISeqFns->first(s);
		Var *v = (Var*)e->key;
		if(!v->dynamic) {
			// printf("Can't dynamically bind non-dynamic var: %s/%s", v->ns, v->sym)
		}
		Validate(v->validator, e->val);
		v->threadBound = true;
		bmap = bmap->obj.fns->IMapFns->assoc(bmap, (const lisp_object*)v, e->val);
		printf("bmap = %p\n", (void*)bmap);
		if(bmap)
			printf("bmap->type = %d\n", bmap->obj.type);
		fflush(stdout);
	}
	dval = NewFrame(bmap, f);
	printf("dval = %p\n", (void*)dval);
	printf("dval->bindings = %p\n", (void*)dval->bindings);
	fflush(stdout);
}

void popThreadBindings(void) {
	printf("In popThreadBindings\n");
	printf("dval = %p\n", (void*)dval);
	if(dval)
		printf("dval->bindings = %p\n", (void*)dval->bindings);
	fflush(stdout);
	const Frame *f = dval->prev;
	if(f == NULL) {
		exception e = {IllegalStateException, "Pop without matching push"};
		Raise(e);
	}
	dval = f;
	printf("dval = %p\n", (void*)dval);
	if(dval)
		printf("dval->bindings = %p\n", (void*)dval->bindings);
	fflush(stdout);
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
	const lisp_object *ret = f->obj.fns->IFnFns->applyTo(f, args);
	return ret;
}

static const char* toStringVar(const lisp_object *obj) {
	assert(obj->type == VAR_type);
	const Var *v = (Var*)obj;
	StringWriter *sw = NewStringWriter();

	if(v->ns) {
		AddString(sw, "#'");
		AddString(sw, toString((lisp_object*)getNameNamespace(v->ns)));
		AddChar(sw, '/');
		AddString(sw, toString((lisp_object*)v->sym));
		return WriteString(sw);
	}

	AddString(sw, "#<Var: ");
	AddString(sw, (v->sym ? toString((lisp_object*)v->sym) : "--unnamed--"));
	AddChar(sw, '>');
	return WriteString(sw);
}
