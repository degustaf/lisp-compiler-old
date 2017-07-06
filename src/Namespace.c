#include "Namespace.h"

#include <stdio.h>	// For Debugging.

#include "Error.h"
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

Namespace *findNS(const Symbol *s) {
	const MapEntry *me = namespaces->obj.fns->IMapFns->entryAt(namespaces, (lisp_object*)s);
	if(me == NULL)
		return NULL;
	const lisp_object *ret = me->val;
	assert(ret->type == NAMESPACE_type);
	return (Namespace*)ret;
}

Namespace *findOrCreateNS(const Symbol *s) {
	const MapEntry *me = namespaces->obj.fns->IMapFns->entryAt(namespaces, (const lisp_object*)s);
	const lisp_object *obj = me ? me->val: NULL;
	if(obj != NULL) {
		assert(obj->type == NAMESPACE_type);
		return (Namespace*) obj;
	}
	Namespace *NewNS = NewNamespace(s);
	namespaces = namespaces->obj.fns->IMapFns->assoc(namespaces, (const lisp_object*)s, (const lisp_object*)NewNS);
	return NewNS;
}

Namespace *lookupAlias(Namespace *ns, const Symbol *alias) {
	const IMap *map = ns->aliases;
	const MapEntry *me = map->obj.fns->IMapFns->entryAt(map, (lisp_object*)alias);
	const lisp_object *ret = me ? me->val : NULL;
	assert(ret->type == NAMESPACE_type);
	return (Namespace*)ret;
}

Namespace* namespaceFor(Namespace *inns, const Symbol *sym) {
	printf("inns = %p\n", (void*)inns);
	if(inns)
		printf("inns = %s\n", toString((lisp_object*)inns));
	printf("sym = %p\n", (void*)sym);
	if(sym)
		printf("sym = %s\n", toString((lisp_object*)sym));
	fflush(stdout);

	const Symbol *nsSym = internSymbol1(getNamespaceSymbol(sym));
	Namespace *ns = lookupAlias(inns, nsSym);
	if(ns)
		return ns;
	return findNS(nsSym);
}

const Symbol* getNameNamespace(const Namespace *ns) {
	return ns->name;
}

Var *internNS(Namespace *ns, const Symbol *s) {
	printf("In internNS\n");
	printf("ns = %p\n", (void*)ns);
	if(ns)
		printf("ns = %s\n", toString((lisp_object*)ns));
	printf("s = %p\n", (void*)s);
	if(s)
		printf("s = %s\n", toString((lisp_object*)s));
	fflush(stdout);
	if(getNamespaceSymbol(s)) {
		exception e = {IllegalArgumentException, "Can't intern namespace-qualified symbol"};
		Raise(e);
	}
	const IMap *map = ns->mappings;
	const MapEntry *me = map->obj.fns->IMapFns->entryAt(map, (const lisp_object*)s);
	if(me && me->val)
		return (Var*) me->val;
	Var *v = NewVar(ns, s, NULL);
	ns->mappings = map->obj.fns->IMapFns->assoc(map, (const lisp_object*)s, (const lisp_object*)v);
	return v;
}

Var *findInternedVar(const Namespace *ns, const Symbol *s) {
	const IMap *map = ns->mappings;
	const MapEntry *me = map->obj.fns->IMapFns->entryAt(map, (lisp_object*)s);
	const lisp_object *obj = me ? me->val : NULL;
	if(obj && (obj->type == VAR_type)) {
		Var *ret = (Var*)obj;
	   	if(getNamespaceVar(ret) == ns)
			return ret;
	}
	return NULL;
}

const lisp_object* getMapping(const Namespace *ns, const Symbol *s) {
	const IMap *m = ns->mappings;
	const MapEntry *me = m->obj.fns->IMapFns->entryAt(m, (lisp_object*)s);
	return me ? me->val : NULL;
}

static Namespace *NewNamespace(const Symbol *s) {
	Namespace *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = NAMESPACE_type;
	ret->obj.toString = toStringNamespace;
	ret->obj.fns = &NullInterface;
	ret->name = s;
	ret->mappings = (IMap*) EmptyHashMap;	// TODO This should be DEFAULT_IMPORTS.
	ret->aliases = (IMap*) EmptyHashMap;
	return ret;
}

static const char *toStringNamespace(const lisp_object *obj) {
	assert(obj->type == NAMESPACE_type);
	const Namespace *ns = (Namespace*)obj;
	return toString((lisp_object*)ns->name);
}

const char* toStringMapping(const Namespace *ns) {
	return toString((lisp_object*)ns->mappings);
}
