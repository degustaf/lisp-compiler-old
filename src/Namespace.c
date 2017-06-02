#include "Namespace.h"

#include "gc.h"
#include "Interfaces.h"
#include "Map.h"
#include "Util.h"

struct Namespace_Struct {
	lisp_object obj;
	const Symbol *name;
	const IMap *mappings;
	const IMap *aliases;
};

static const IMap *namespaces = (const IMap*) &_EmptyHashMap;

static Namespace *NewNamespace(const Symbol *s);
static const char *toStringNamespace(const lisp_object *);

Namespace *findOrCreateNS(const Symbol *s) {
	const lisp_object *obj = namespaces->obj.fns->IMapFns->entryAt(namespaces, (const lisp_object*)s);
	if(obj != NULL) {
		assert(obj->type == NAMESPACE_type);
		return (Namespace*) obj;
	}
	Namespace *NewNS = NewNamespace(s);
	namespaces = namespaces->obj.fns->IMapFns->assoc(namespaces, (const lisp_object*)s, (const lisp_object*)NewNS);
	return NewNS;
}

static Namespace *NewNamespace(const Symbol *s) {
	Namespace *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = NAMESPACE_type;
	ret->obj.toString = toStringNamespace;
	ret->obj.copy = NULL;	// TODO
	ret->obj.fns = &NullInterface;
	ret->name = s;
	ret->mappings = (IMap*) EmptyHashMap;	// TODO This should be DEFAULT_IMPORTS.
	ret->aliases = (IMap*) EmptyHashMap;
	return ret;
}

Var *internNS(Namespace *ns, const Symbol *s) {
	if(getNamespaceSymbol(s)) {
		NULL;	// throw new IllegalArgumentException("Can't intern namespace-qualified symbol");
	}
	const IMap *map = ns->mappings;
	const lisp_object *o = NULL;
	if((o = map->obj.fns->IMapFns->entryAt(map, (const lisp_object*)s)))
		return (Var*) o;
	Var *v = NewVar(ns, s, NULL);
	ns->mappings = map->obj.fns->IMapFns->assoc(map, (const lisp_object*)s, (const lisp_object*)v);
	return v;
}

static const char *toStringNamespace(const lisp_object *obj) {
	assert(obj->type == NAMESPACE_type);
	const Namespace *ns = (Namespace*)obj;
	return toString((lisp_object*)ns->name);
}
