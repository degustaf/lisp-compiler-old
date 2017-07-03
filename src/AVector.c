#include "AVector.h"

#include "ASeq.h"
#include "Error.h"
#include "gc.h"
#include "Numbers.h"
#include "Util.h"
#include "Vector.h"

typedef struct {
	lisp_object obj;
	const IVector *v;
	size_t i;
} RSeq;

static const lisp_object* firstRSeq(const ISeq*);
static const ISeq* nextRSeq(const ISeq*);
static size_t countRSeq(const ICollection*);
static RSeq* NewRSeq(const IVector*, size_t);

const Seqable_vtable RSeq_Seqable_vtable = {
	seqASeq,	//seq
};

const ICollection_vtable RSeq_ICollection_vtable = {
	countRSeq,					// count
	(ICollectionFn1)consASeq,	// cons
	emptyASeq,					// empty
	EquivASeq					// Equiv
};

const ISeq_vtable RSeq_ISeq_vtable = {
	firstRSeq,	// first
	nextRSeq,	// next
	moreASeq,	// more
	consASeq,	// cons
};

interfaces RSeq_interfaces = {
	&RSeq_Seqable_vtable,		// SeqableFns
	NULL,						// ReversibleFns
	&RSeq_ICollection_vtable,	// ICollectionFns
	NULL,						// IStackFns
	&RSeq_ISeq_vtable,			// ISeqFns
	NULL,						// IFnFns
	NULL,						// IVectorFns
	NULL,						// IMapFns
};

static const lisp_object* firstRSeq(const ISeq *self) {
	assert(((lisp_object*)self)->type == RSEQ_type);
	const RSeq *rs = (RSeq*)self;
	return rs->v->obj.fns->IVectorFns->nth(rs->v, rs->i, NULL);
}

static const ISeq* nextRSeq(const ISeq *self) {
	assert(((lisp_object*)self)->type == RSEQ_type);
	const RSeq *rs = (RSeq*)self;
	if(rs->i > 0)
		return (ISeq*)NewRSeq(rs->v, rs->i-1);
	return NULL;
}

static size_t countRSeq(const ICollection *self) {
	assert(((lisp_object*)self)->type == RSEQ_type);
	const RSeq *rs = (RSeq*)self;
	return rs->i + 1;
}

static RSeq* NewRSeq(const IVector *v, size_t i) {
	RSeq *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = RSEQ_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.toString = toString;
	ret->obj.Equals = Equals;
	ret->obj.fns = &RSeq_interfaces;

	ret->v = v;
	ret->i = i;

	return ret;
}

const ISeq* rseq(const Seqable *s) {
	assert(isIVector((lisp_object*)s));
	const IVector *iv = (IVector*)s;
	size_t count = iv->obj.fns->ICollectionFns->count((ICollection*)iv);
	if(count > 0)
		return (ISeq*)NewRSeq(iv, count - 1);
	return NULL;
}

const ICollection* emptyAVector(void) {
	return (ICollection*)EmptyVector;
}

bool EquivAVector(const ICollection *ic, const lisp_object *obj) {
	assert(isIVector((lisp_object*)ic));
	const IVector *v = (IVector*)ic;
	if((lisp_object*)ic == obj)
		return true;
	if(count((lisp_object*)v) != count(obj))
		return false;

	if(isIVector(obj)) {
		IVector *ov = (IVector*)obj;
		for(size_t i = 0; i <count(obj); i++)
			if(!Equiv(v->obj.fns->IVectorFns->nth(v, i, NULL), ov->obj.fns->IVectorFns->nth(ov, i, NULL)))
				return false;
		return true;
	}

	if(!isSeqable(obj))
		return false;

	const ISeq *ms = seq(obj);
	for(size_t i = 0; i < count((lisp_object*)v); i++, ms = ms->obj.fns->ISeqFns->next(ms))
		if(!Equiv(v->obj.fns->IVectorFns->nth(v, i, NULL), ms->obj.fns->ISeqFns->first(ms)))
			return false;

	return true;
}

bool EqualsAVector(const struct lisp_object_struct *x, const struct lisp_object_struct *y) {
	assert(isIVector(x));
	const IVector *vx = (IVector*)x;
	if(x == y)
		return true;

	if(isIVector(x)) {
		const IVector *vy = (IVector*)y;
		size_t count = vx->obj.fns->ICollectionFns->count((ICollection*)vx);
		if(count != vy->obj.fns->ICollectionFns->count((ICollection*)vy))
			return false;
		for(size_t i = 0; i < count; i++)
			if(!Equals(vx->obj.fns->IVectorFns->nth(vx, i, NULL), vx->obj.fns->IVectorFns->nth(vx, i, NULL)))
				return false;
		return true;
	}

	if(!isSeqable(y))
		return false;

	const ISeq *ms = seq(y);
	for(size_t i = 0; i < count((lisp_object*)vx); i++, ms = ms->obj.fns->ISeqFns->next(ms))
		if(!Equals(vx->obj.fns->IVectorFns->nth(vx, i, NULL), ms->obj.fns->ISeqFns->first(ms)))
			return false;

	return true;
}

const lisp_object* peekAVector(const IStack *is) {
	assert(isIVector((lisp_object*)is));
	const IVector *iv = (IVector*)is;
	size_t count = iv->obj.fns->ICollectionFns->count((ICollection*)iv);
	if(count > 0)
		return iv->obj.fns->IVectorFns->nth(iv, count, NULL);
	return NULL;
}

const lisp_object* invoke1AVector(const IFn *f, const lisp_object *arg1) {
	assert(isIVector(&f->obj));
	if(arg1->type == INTEGER_type)
		return f->obj.fns->IVectorFns->nth((IVector*)f, IntegerValue((Integer*)arg1), NULL);
	exception e = {IllegalArgumentException, "Key must be integer"};
	Raise(e);
	__builtin_unreachable();
}

size_t lengthAVector(const IVector *iv) {
	return iv->obj.fns->ICollectionFns->count((ICollection*)iv);
}

const IVector* assocAVector(const IVector *iv, const lisp_object *key, const lisp_object *val) {
	if(key->type == INTEGER_type) {
		return iv->obj.fns->IVectorFns->assocN(iv, IntegerValue((Integer*)key), val);
	}
	exception e = {IllegalArgumentException, "Key must be integer"};
	Raise(e);
	__builtin_unreachable();
}

const MapEntry* entryAtAVector(const IVector *iv, const lisp_object *key) {
	if(key->type == INTEGER_type) {
		size_t i = IntegerValue((Integer*)key);
		if(i < iv->obj.fns->ICollectionFns->count((ICollection*)iv)) {
			
			return NewMapEntry(key, iv->obj.fns->IVectorFns->nth(iv, i, NULL));
		}
	}

	return NULL;
}
