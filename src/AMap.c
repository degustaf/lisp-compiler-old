#include "AMap.h"

#include "ASeq.h"
#include "gc.h"
#include "Map.h"
#include "MapEntry.h"
#include "Util.h"

struct KeySeq_struct {
	lisp_object obj;
	const ISeq *seq;
	const IMap *map;
};

static const lisp_object* firstKeySeq(const ISeq*);
static const ISeq* nextKeySeq(const ISeq*);
static const KeySeq* NewKeySeq(const ISeq *seq, const IMap *map);

const Seqable_vtable KeySeq_Seqable_vtable = {
	seqASeq,	//seq
};

const ICollection_vtable KeySeq_ICollection_vtable = {
	countASeq,					// count
	(ICollectionFn1)consASeq,	// cons
	emptyASeq,					// empty
	EquivASeq					// Equiv
};

const ISeq_vtable KeySeq_ISeq_vtable = {
	firstKeySeq,	// first
	nextKeySeq,		// next
	moreASeq,		// more
	consASeq,		// cons
};

interfaces KeySeq_interfaces = {
	&KeySeq_Seqable_vtable,		// SeqableFns
	NULL,						// ReversibleFns
	&KeySeq_ICollection_vtable,	// ICollectionFns
	NULL,						// IStackFns
	&KeySeq_ISeq_vtable,		// ISeqFns
	NULL,						// IFnFns
	NULL,						// IVectorFns
	NULL,						// IMapFns
};

const KeySeq* CreateKeySeq(const ISeq *seq) {
	if(seq == NULL)
		return NULL;
	return NewKeySeq(seq, NULL);
}

const KeySeq* CreateKeySeqFromMap(const IMap *map) {
	if(map == NULL)
		return NULL;
	const ISeq *seq = map->obj.fns->SeqableFns->seq((Seqable*)map);
	if(seq == NULL)
		return NULL;
	return NewKeySeq(seq, map);
}

static const lisp_object* firstKeySeq(const ISeq *is) {
	assert(is->obj.type == KEYSEQ_type);
	const KeySeq *ks = (KeySeq*)is;
	MapEntry *e = (MapEntry*)ks->seq->obj.fns->ISeqFns->first(ks->seq);
	return e->key;
}

static const ISeq* nextKeySeq(const ISeq *is) {
	assert(is->obj.type == KEYSEQ_type);
	const KeySeq *ks = (KeySeq*)is;
	return (ISeq*)CreateKeySeq(ks->seq->obj.fns->ISeqFns->next(ks->seq));
}

static const KeySeq* NewKeySeq(const ISeq *seq, const IMap *map) {
	KeySeq *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = KEYSEQ_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.toString = toString;
	ret->obj.Equals = Equals;
	ret->obj.fns = &KeySeq_interfaces;

	ret->seq = seq;
	ret->map = map;

	return ret;
}


struct ValSeq_struct {
	lisp_object obj;
	const ISeq *seq;
	const IMap *map;
};

static const lisp_object* firstValSeq(const ISeq*);
static const ISeq* nextValSeq(const ISeq*);
static const ValSeq* NewValSeq(const ISeq *seq, const IMap *map);

const Seqable_vtable ValSeq_Seqable_vtable = {
	seqASeq,	//seq
};

const ICollection_vtable ValSeq_ICollection_vtable = {
	countASeq,					// count
	(ICollectionFn1)consASeq,	// cons
	emptyASeq,					// empty
	EquivASeq					// Equiv
};

const ISeq_vtable ValSeq_ISeq_vtable = {
	firstValSeq,	// first
	nextValSeq,		// next
	moreASeq,		// more
	consASeq,		// cons
};

interfaces ValSeq_interfaces = {
	&ValSeq_Seqable_vtable,		// SeqableFns
	NULL,						// ReversibleFns
	&ValSeq_ICollection_vtable,	// ICollectionFns
	NULL,						// IStackFns
	&ValSeq_ISeq_vtable,		// ISeqFns
	NULL,						// IFnFns
	NULL,						// IVectorFns
	NULL,						// IMapFns
};

const ValSeq* CreateValSeq(const ISeq *seq) {
	if(seq == NULL)
		return NULL;
	return NewValSeq(seq, NULL);
}

const ValSeq* CreateValSeqFromMap(const IMap *map) {
	if(map == NULL)
		return NULL;
	const ISeq *seq = map->obj.fns->SeqableFns->seq((Seqable*)map);
	if(seq == NULL)
		return NULL;
	return NewValSeq(seq, map);
}

static const lisp_object* firstValSeq(const ISeq *is) {
	assert(is->obj.type == KEYSEQ_type);
	const ValSeq *ks = (ValSeq*)is;
	MapEntry *e = (MapEntry*)ks->seq->obj.fns->ISeqFns->first(ks->seq);
	return e->key;
}

static const ISeq* nextValSeq(const ISeq *is) {
	assert(is->obj.type == KEYSEQ_type);
	const ValSeq *ks = (ValSeq*)is;
	return (ISeq*)CreateValSeq(ks->seq->obj.fns->ISeqFns->next(ks->seq));
}

static const ValSeq* NewValSeq(const ISeq *seq, const IMap *map) {
	ValSeq *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = KEYSEQ_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.toString = toString;
	ret->obj.Equals = Equals;
	ret->obj.fns = &ValSeq_interfaces;

	ret->seq = seq;
	ret->map = map;

	return ret;
}
