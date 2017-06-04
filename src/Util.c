#include "Util.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "Bool.h"
#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.
#include "gc.h"
#include "Interfaces.h"
#include "List.h"
#include "Map.h"
#include "Numbers.h"
#include "Murmur3.h"
#include "Symbol.h"
#include "Vector.h"

bool EqualBase(const lisp_object *x, const lisp_object *y) {
	return x == y;
}

bool Equals(const lisp_object *x, const lisp_object *y) {
	if(x == y)
		return true;
	return x && x->Equals(x, y);
}

bool Equiv(const lisp_object *x, const lisp_object *y) {
	if(x == y)
		return true;
	if(x) {
		// TODO	Extend Equiv
		// If x and y are Numbers.
			// return Numbers.Equal(x, y)
		if(isICollection(x)) {
			return x->fns->ICollectionFns->Equiv((const ICollection*)x, y);
		}
		if(x && isICollection(y)) {
			return y->fns->ICollectionFns->Equiv((const ICollection*)y, x);
		}
		// return x.equals(y)
	}
	return false;
}

uint32_t HashEq(const lisp_object *x) {
	if(x == NULL) return 0;

	switch(x->type) {
		case CHAR_type:
		case STRING_type: {
			const char *s = toString(x);
			return hash32(s, strlen(s));
		}
		case INTEGER_type: {
			long i = IntegerValue((Integer*)x);
			return hash32(&i, sizeof(i));
		}
		case FLOAT_type: {
			double y = FloatValue((Float*)x);
			return hash32(&y, sizeof(y));
		}
		case SYMBOL_type: {
			const Symbol *s = (Symbol*) x;
			const char *name = getNameSymbol(s);
			const char *ns = getNamespaceSymbol(s);
			return hashCombine(hash32(name, strlen(name)), hash32(ns, ns ? strlen(ns) : 0));
		}
		case LIST_type:			// TODO HashEq
		case CONS_type:			// TODO HashEq
		case NODESEQ_type:		// TODO HashEq
		case ARRAYNODESEQ_type:	// TODO HashEq
		case HASHMAP_type:		// TODO HashEq
		case VECTOR_type:		// TODO HashEq
		default:
			return hash32(&x, sizeof(x));
	}
}

uint32_t hashCombine(uint32_t x, uint32_t y) {
	return x ^ (y + HASH_MIXER + (x << 6) + (x >> 2));
}

static void PrintObject(StringWriter *sw, const lisp_object *obj) {
	assert(sw);
	if((void*) obj == (void*) EmptyList) {
		AddString(sw, "()");
		return;
	}
	if(isISeq(obj)) {
		assert(obj->fns->ISeqFns->next != NULL);
		assert(obj->fns->ISeqFns->first != NULL);

		AddChar(sw, '(');

    	for(const ISeq *list = (ISeq*)obj; list != NULL; list = list->obj.fns->ISeqFns->next(list)) {
    	    const lisp_object *o = list->obj.fns->ISeqFns->first(list);
			PrintObject(sw, o);
			AddChar(sw, ' ');
    	}
		Shrink(sw, 1);
		AddChar(sw, ')');
		return;
	}
	if(isIMap(obj)) {
		if(obj->fns->ICollectionFns->count((const ICollection*)obj) == 0) {
			AddString(sw, "{}");
			return;
		}

		AddChar(sw, '{');

		const ISeq *s = obj->fns->SeqableFns->seq((Seqable*)obj);
		for(; s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
			const MapEntry *e = (MapEntry*) s->obj.fns->ISeqFns->first(s);
			PrintObject(sw, e->key);
			AddChar(sw, ' ');
			PrintObject(sw, e->val);
			AddString(sw, ", ");
		}
		Shrink(sw, 2);
		AddChar(sw, '}');
		return;
	}
	if(isIVector(obj)) {
		if(obj == (const lisp_object*) EmptyVector) {
			AddString(sw, "[]");
			return;
		}
		AddChar(sw, '[');
		for(size_t i = 0; i < obj->fns->ICollectionFns->count((const ICollection*)obj); i++) {
			PrintObject(sw, obj->fns->IVectorFns->nth((const IVector*)obj, i, NULL));
			AddChar(sw, ' ');
		}
		Shrink(sw, 1);
		AddChar(sw, ']');
		return;
	}
	AddString(sw, obj->toString(obj));
}

const char *toString(const lisp_object *obj) {
	StringWriter *sw = NewStringWriter();
	PrintObject(sw, obj);
	return WriteString(sw);
}

const lisp_object *get(const lisp_object *coll, const lisp_object *key, const lisp_object *NotFound) {
	if(coll == NULL) return NULL;

	const lisp_object *ret = NULL;
	if(isIMap(coll)) {
		ret = coll->fns->IMapFns->entryAt((IMap*)coll, key);
	} else if(isIVector(coll)) {
		ret = coll->fns->IVectorFns->entryAt((IVector*)coll, key);
	}
	return ret ? ret : NotFound;
}

const ISeq *seq(const lisp_object *obj) {
	if(obj == NULL)
		return NULL;
	if(isSeqable(obj))
		return obj->fns->SeqableFns->seq((const Seqable*) obj);

	return (const ISeq*) NewError(0, "Don't know how to create ISeq from: %s.", object_type_string[obj->type]);
}

bool boolCast(const lisp_object *obj) {
	if(obj->type == BOOL_type)
		return obj == (lisp_object*)True;
	return obj != NULL;
}

const lisp_object *withMeta(const lisp_object *obj, const IMap *meta) {
	lisp_object *ret = GC_MALLOC(obj->size);
	memcpy(ret, obj, obj->size);
	ret->meta = meta;
	return ret;
}

size_t count(const lisp_object *obj) {
	if(obj == NULL)
		return 0;
	if(isICollection(obj))
		return obj->fns->ICollectionFns->count((ICollection*)obj);
	// TODO // throw "Count not supported on this type."
	return 0;
}
