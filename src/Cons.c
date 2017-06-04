#include "Cons.h"

#include <assert.h>
#include <stddef.h>

#include "ASeq.h"
#include "gc.h"
#include "List.h"


struct Cons_struct {
	lisp_object obj;
	const lisp_object *_first;
	const ISeq *_more;
};

static const lisp_object* firstCons(const ISeq*);
static const ISeq* nextCons(const ISeq*);
static const ISeq* moreCons(const ISeq*);
static size_t countCons(const ICollection*);

Seqable_vtable Cons_Seqable_vtable = {
	seqASeq,	//seq
};

ICollection_vtable Cons_ICollection_vtable = {
	countCons,	// count
	emptyASeq,	// empty
	EquivASeq	// Equiv
};

ISeq_vtable Cons_ISeq_vtable = {
	firstCons,	// first
	nextCons,	// next
	moreCons,	// more
	consASeq,	// cons
};

interfaces Cons_interfaces = {
	&Cons_Seqable_vtable,		// SeqableFns
	NULL,						// ReversibleFns
	&Cons_ICollection_vtable,	// ICollectionFns
	NULL,						// IStackFns
	&Cons_ISeq_vtable,			// ISeqFns
	NULL,						// IFnFns
	NULL,						// IVectorFns
	NULL,						// IMapFns
};

const Cons *NewCons(const lisp_object *obj, const ISeq *s) {
	assert(isISeq(&s->obj));
	Cons *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = CONS_type;
	ret->obj.size = sizeof(Cons);
	ret->obj.fns = &Cons_interfaces;
	ret->_first = obj;
	ret->_more = s;
	return (const Cons*) ret;
}

static const lisp_object* firstCons(const ISeq *s) {
	assert(s->obj.type == CONS_type);
	const Cons *c = (const Cons*) s;
	return c->_first;
}

static const ISeq* nextCons(const ISeq *s) {
	assert(s->obj.type == CONS_type);
	const ISeq *s2 = s->obj.fns->ISeqFns->more(s);
	return s2->obj.fns->SeqableFns->seq((const Seqable*)s2);
}

static const ISeq* moreCons(const ISeq *s) {
	assert(s->obj.type == CONS_type);
	const Cons *c = (const Cons*) s;
	if(c->_more == NULL) return (const ISeq*) EmptyList;
	return c->_more;
}

static size_t countCons(const ICollection *s) {
	assert(s->obj.type == CONS_type);
	const Cons *c = (const Cons*) s;
	return 1 + c->_more->obj.fns->ICollectionFns->count((const ICollection*)c->_more);
}
