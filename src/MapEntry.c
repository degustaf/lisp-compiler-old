#include "MapEntry.h"

#include "AFn.h"
#include "AVector.h"
#include "Error.h"
#include "gc.h"
#include "Interfaces.h"
#include "Util.h"
#include "Vector.h"

static const ISeq* seqMapEntry(const Seqable*);
static size_t countMapEntry(const ICollection*);
static const ICollection* emptyMapEntry(void);
static const IStack* popMapEntry(const IStack*);
static size_t lengthMapEntry(const IVector*);
static const IVector* assocNMapEntry(const IVector*, size_t, const lisp_object*);
static const IVector* consMapEntry(const IVector*, const lisp_object*);
static const lisp_object* nthMapEntry(const IVector*, size_t n, const lisp_object *NotFound);

const Seqable_vtable MapEntry_Seqable_vtable = {
	seqMapEntry, // seq
};

const Reversible_vtable MapEntry_Reversible_vtable = {
	rseq, // rseq
};

const ICollection_vtable MapEntry_ICollection_vtable = {
	countMapEntry,					// count
	(ICollectionFn1) consMapEntry,	// cons
	emptyMapEntry,					// empty
	EquivAVector,					// Equiv
};

const IStack_vtable MapEntry_IStack_vtable = {
	peekAVector,	// peek
	popMapEntry,	// pop 
};

const IFn_vtable MapEntry_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1AVector,	// invoke1
	invoke2AFn,		// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
};

const IVector_vtable MapEntry_IVector_vtable = {
	lengthMapEntry,		// length
	assocNMapEntry,		// assocN
	consMapEntry,		// cons
	assocAVector,		// assoc
	entryAtAVector,		// entryAt
	nthMapEntry,		// nth
};

interfaces MapEntry_interfaces = {
	&MapEntry_Seqable_vtable,		// SeqableFns
	&MapEntry_Reversible_vtable,	// ReversibleFns
	&MapEntry_ICollection_vtable,	// ICollectionFns
	&MapEntry_IStack_vtable,		// IStackFns
	NULL,							// ISeqFns
	&MapEntry_IFn_vtable,			// IFnFns
	&MapEntry_IVector_vtable,		// IVectorFns
	NULL,							// IMapFns
};

const MapEntry* NewMapEntry(const lisp_object *key, const lisp_object *val) {
	MapEntry *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = MAPENTRY_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.toString = toString;
	ret->obj.Equals = EqualsAVector;
	ret->obj.fns = &MapEntry_interfaces;

	ret->key = key;
	ret->val = val;
	return ret;
}

static const IVector* asVector(const MapEntry *me) {
	const lisp_object *entries[2] = {me->key, me->val};
	return (IVector*) CreateVector(2, entries);
}

static const ISeq* seqMapEntry(const Seqable *s) {
	assert(s->obj.type == MAPENTRY_type);
	const IVector *v = asVector((MapEntry*)s);
	return v->obj.fns->SeqableFns->seq((Seqable*)v);
}

static size_t countMapEntry(const ICollection *ic) {
	assert(ic->obj.type == MAPENTRY_type);
	return 2;
}

static const ICollection* emptyMapEntry(void) {
	return NULL;
}

static const IStack* popMapEntry(const IStack *is) {
	assert(is->obj.type == MAPENTRY_type);
	const MapEntry *me = (MapEntry*) is;
	return (IStack*) CreateVector(1, &me->key);
}

static size_t lengthMapEntry(const IVector *iv) {
	assert(iv->obj.type == MAPENTRY_type);
	return 2;
}

static const IVector* assocNMapEntry(const IVector *iv, size_t i, const lisp_object *val) {
	assert(iv->obj.type == MAPENTRY_type);
	iv = asVector((MapEntry*)iv);
	return iv->obj.fns->IVectorFns->assocN(iv, i, val);
}

static const IVector* consMapEntry(const IVector *iv, const lisp_object *o) {
	assert(iv->obj.type == MAPENTRY_type);
	iv = asVector((MapEntry*)iv);
	return iv->obj.fns->IVectorFns->cons(iv, o);
}

static const lisp_object* nthMapEntry(const IVector *iv, size_t n, __attribute__((unused)) const lisp_object *NotFound) {
	assert(iv->obj.type == MAPENTRY_type);
	switch(n) {
		case 0:
			return ((MapEntry*)iv)->key;
		case 1:
			return ((MapEntry*)iv)->val;
		default:
			break;
	}
	exception e = {IndexOutOfBoundException, NULL};
	Raise(e);
	__builtin_unreachable();
}
