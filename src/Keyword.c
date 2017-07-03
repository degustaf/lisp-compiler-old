#include "Keyword.h"

#include "AFn.h"
#include "gc.h"
#include "Interfaces.h"
#include "Map.h"
#include "StringWriter.h"
#include "Util.h"

struct Keyword_struct {
	lisp_object obj;
	const Symbol *sym;
};

static const IMap *Cache = (const IMap*) &_EmptyHashMap;

static const Keyword* NewKeyword(const Symbol* s);
static const lisp_object* invoke1Keyword(const IFn*, const lisp_object*);
static const lisp_object* invoke2Keyword(const IFn*, const lisp_object*, const lisp_object*);
static const char *toStringKeyword(const lisp_object *);

const IFn_vtable Keyword_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1Keyword,	// invoke1
	invoke2Keyword,	// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
};

interfaces Keyword_interfaces = {
	NULL,					// SeqableFns
	NULL,					// ReversibleFns
	NULL,					// ICollectionFns
	NULL,					// IStackFns
	NULL,					// ISeqFns
	&Keyword_IFn_vtable,	// IFnFns
	NULL,					// IVectorFns
	NULL,					// IMapFns
};

const Keyword _arglistsKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_arglistsSymbol};
const Keyword *const arglistsKW = &_arglistsKW;
const Keyword _ColumnKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_ColumnSymbol};
const Keyword *const ColumnKW = &_ColumnKW;
const Keyword _ConstKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_ConstSymbol};
const Keyword *const ConstKW = &_ConstKW;
const Keyword _DocKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_DocSymbol};
const Keyword *const DocKW = &_DocKW;
const Keyword _DynamicKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_DynamicSymbol};
const Keyword *const DynamicKW = &_DynamicKW;
const Keyword _FileKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_FileSymbol};
const Keyword *const FileKW = &_FileKW;
const Keyword _LineKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_LineSymbol};
const Keyword *const LineKW = &_LineKW;
const Keyword _macroKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_macroSymbol};
const Keyword *const macroKW = &_macroKW;
const Keyword _privateKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_privateSymbol};
const Keyword *const privateKW = &_privateKW;
const Keyword _tagKW = {{KEYWORD_type, sizeof(Keyword), toStringKeyword, EqualBase, (IMap*) &_EmptyHashMap, &Keyword_interfaces}, &_tagSymbol};
const Keyword *const tagKW = &_tagKW;

const Keyword *internKeyword(const Symbol *s) {
	const lisp_object *obj = Cache->obj.fns->IMapFns->entryAt(Cache, (lisp_object*) s)->val;
	if(obj) {
		assert(obj->type == KEYWORD_type);
		return (const Keyword*) obj;
	}
	const Keyword *k = NewKeyword(s);
	Cache = Cache->obj.fns->IMapFns->assoc(Cache, (const lisp_object*) s, (const lisp_object*) k);
	return k;
}

const Keyword *internKeyword1(const char *nsname) {
	return internKeyword(internSymbol1(nsname));
}

const Keyword *internKeyword2(const char *ns, const char *name) {
	return internKeyword(internSymbol2(ns, name));
}

const char *getNameKeyword(const Keyword *k) {
	return getNameSymbol(k->sym);
}

const char *getNamespaceKeyword(const Keyword *k) {
	return getNamespaceSymbol(k->sym);
}

const Keyword *findKeyword(const Symbol *s) {
	const lisp_object *ret = Cache->obj.fns->IMapFns->entryAt(Cache, (const lisp_object*)s)->val;
	if(ret == NULL)
		return NULL;
	assert(ret->type == KEYWORD_type);
	return (Keyword*)ret;
}

const Keyword *findKeyword1(const char *nsname) {
	return findKeyword(internSymbol1(nsname));
}

const Keyword *findKeyword2(const char *ns, const char *name) {
	return findKeyword(internSymbol2(ns, name));
}

void removeKWfromCache(void *obj, void *client_data) {
	const lisp_object *kw = (lisp_object*)obj;
	const IMap **cache_address = (const IMap* *)client_data;
	*cache_address = (*cache_address)->obj.fns->IMapFns->without(*cache_address, kw);
}

static const Keyword* NewKeyword(const Symbol* s) {
	Keyword *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = KEYWORD_type;
	ret->obj.size = sizeof(Keyword);
	ret->obj.toString = toStringKeyword;
	ret->obj.Equals = EqualBase;
	ret->obj.fns = &Keyword_interfaces;
	ret->sym = s;

	GC_REGISTER_FINALIZER(ret, removeKWfromCache, &Cache, NULL, NULL);

	return ret;
}

static const lisp_object* invoke1Keyword(const IFn *k, const lisp_object *obj) {
	return get(obj, (const lisp_object*)k, NULL);
}

static const lisp_object* invoke2Keyword(const IFn *k, const lisp_object *obj, const lisp_object *NotFound) {
	return get(obj, (const lisp_object*)k, NotFound);
}

static const char *toStringKeyword(const lisp_object *obj) {
	assert(obj->type == KEYWORD_type);
	const Keyword *k = (Keyword*)obj;
	StringWriter *sw = NewStringWriter();
	AddChar(sw, ':');
	const lisp_object *s = (lisp_object*)k->sym;
	AddString(sw, s->toString(s));
	return WriteString(sw);
}
