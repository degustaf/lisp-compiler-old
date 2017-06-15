#include "Symbol.h"

#include <assert.h>
#include <string.h>

#include "AFn.h"
#include "gc.h"
#include "Interfaces.h"
#include "Map.h"
#include "StringWriter.h"
#include "Util.h"

struct Symbol_struct {
	lisp_object obj;
	const char *ns;
	const char *name;
};

const lisp_object* invoke1Symbol(const IFn*, const lisp_object*);
const lisp_object* invoke2Symbol(const IFn*, const lisp_object*, const lisp_object*);
const Symbol* newSymbol(const char *ns, const char *name);
const char *toStringSymbol(const lisp_object *);
static bool EqualSymbol(const lisp_object *x, const lisp_object *y);

IFn_vtable Symbol_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1Symbol,	// invoke1
	invoke2Symbol,	// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
};

interfaces Symbol_interfaces = {
	NULL,				// SeqableFns
	NULL,				// ReversibleFns
	NULL,				// ICollectionFns
	NULL,				// IStackFns
	NULL,				// ISeqFns
	&Symbol_IFn_vtable,	// IFnFns
	NULL,				// IVectorFns
	NULL,				// IMapFns
};

const Symbol _arglistsSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "arglists"};
const Symbol _ConstSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "const"};
const Symbol _DefSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "def"};
const Symbol _DocSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "doc"};
const Symbol _DynamicSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "dynamic"};
const Symbol _FileSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "file"};
const Symbol _FnOnceSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "fn*"};
const Symbol _LetSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "let*"};
const Symbol _LoopSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "loop"};
const Symbol _macroSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "macro"};
const Symbol _privateSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "private"};
const Symbol _quoteSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "quote"};
const Symbol _tagSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "tag"};

const Symbol *FnOnceSymbol = &_FnOnceSymbol;
const Symbol *const LoopSymbol = &_LoopSymbol;
const Symbol *const quoteSymbol = &_quoteSymbol;

const Symbol _AmpSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "&"};
const Symbol *const AmpSymbol = &_AmpSymbol;
const Symbol _derefSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "deref"};
const Symbol *const derefSymbol = &_derefSymbol;
const Symbol _DoSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "do"};
const Symbol *const DoSymbol = &_DoSymbol;
const Symbol _FNSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "fn*"};
const Symbol *const FNSymbol = &_FNSymbol;
const Symbol _inNamespaceSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "inNamespace"};
const Symbol *const inNamespaceSymbol = &_inNamespaceSymbol;
const Symbol _in_nsSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "in-ns"};
const Symbol *const in_nsSymbol = &_inNamespaceSymbol;
const Symbol _loadFileSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "loadFile"};
const Symbol *const loadFileSymbol = &_loadFileSymbol;
const Symbol _namespaceSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "namespace"};
const Symbol *const namespaceSymbol = &_namespaceSymbol;
const Symbol _nsSymbol = {{SYMBOL_type, sizeof(Symbol), toStringSymbol, NULL, EqualSymbol, (IMap*)&_EmptyHashMap, &Symbol_interfaces}, NULL, "ns"};
const Symbol *const nsSymbol = &_nsSymbol;

const Symbol *internSymbol1(const char *nsname) {
	size_t len = strlen(nsname);
	char *split = strrchr(nsname, '/');
	if(split == NULL || len == 1)
		return newSymbol(NULL, nsname);
	char *ns = GC_MALLOC_ATOMIC(split - nsname + 1);
	strncpy(ns, nsname, split - nsname);
	ns[split - nsname] = '\0';
	char *name = GC_MALLOC_ATOMIC(len - (split - nsname));
	split++;
	strncpy(name, split, len - (split - nsname));
	name[len - (split - nsname) - 1] = '\0';
	return newSymbol(ns, name);
}

const Symbol *internSymbol2(const char *ns, const char *name) {
	return newSymbol(ns, name);
}

const char *getNameSymbol(const Symbol *s) {
	return s->name;
}

const char *getNamespaceSymbol(const Symbol *s) {
	return s->ns;
}

const lisp_object* invoke1Symbol(const IFn *s, const lisp_object *arg1) {
	assert(s->obj.type == SYMBOL_type);
	return get(arg1, (const lisp_object*)s, NULL);
}

const lisp_object* invoke2Symbol(const IFn *s, const lisp_object *arg1, const lisp_object *arg2) {
	assert(s->obj.type == SYMBOL_type);
	return get(arg1, (const lisp_object*)s, arg2);
}

const Symbol* newSymbol(const char *ns, const char *name) {
	Symbol *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = SYMBOL_type;
	ret->obj.size = sizeof(Symbol);
	ret->obj.toString =	toStringSymbol;
	// ret->obj.copy =		// TODO
	ret->obj.Equals = EqualSymbol;
	ret->obj.fns = &Symbol_interfaces;
	ret->ns = ns;
	ret->name = name;

	return ret;
}

const char *toStringSymbol(const lisp_object *obj) {
	assert(obj->type == SYMBOL_type);
	Symbol *s = (Symbol*)obj;

	StringWriter *sw = NewStringWriter();
	if(s->ns) {
		AddString(sw, s->ns);
		AddChar(sw, '/');
	}
	AddString(sw, s->name);
	return WriteString(sw);
}

static bool EqualSymbol(const lisp_object *x, const lisp_object *y) {
	assert(x->type == SYMBOL_type);
	const Symbol *xSym = (Symbol*) x;

	if(x == y)
		return true;
	if(y->type != SYMBOL_type)
		return false;
	const Symbol *ySym = (Symbol*) y;
	if(xSym->ns == NULL && ySym->ns == NULL)
		return strcmp(xSym->name, ySym->name);
	if(xSym->ns == NULL || ySym->ns == NULL)
		return false;
	return strcmp(xSym->ns, ySym->ns) && strcmp(xSym->name, ySym->name);
}
