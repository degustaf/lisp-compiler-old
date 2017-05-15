#include "ASeq.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "Cons.h"
#include "List.h"
#include "Util.h"


const ISeq *moreASeq(const ISeq *obj) {
	assert(isISeq(&obj->obj));
	assert(obj->obj.fns->ISeqFns->next != NULL);

	const ISeq *s = obj->obj.fns->ISeqFns->next(obj);
	if(s) return s;
	return (ISeq*) EmptyList;
}

const ISeq *consASeq(const ISeq *s, const lisp_object *obj) {
	assert(isISeq(&s->obj));

	return (ISeq*) NewCons(obj, s);
}

size_t countASeq(const ICollection *s) {
	assert(isISeq(&s->obj));
	
	if(s == NULL) return 0;
	const ISeq *s2 = s->obj.fns->ISeqFns->next((const ISeq*)s);
	return 1 + s2->obj.fns->ICollectionFns->count((const ICollection*)s2);
}

const ICollection* emptyASeq(void) {
	return (const ICollection*) EmptyList;
}

bool EquivASeq(const ICollection *is, const lisp_object *obj) {
	assert(isISeq(&is->obj));
	assert(is->obj.fns->ISeqFns->first != NULL);
	assert(is->obj.fns->ISeqFns->next != NULL);

	if(!isISeq(obj)) return false;
	const ISeq *ms = obj->fns->SeqableFns->seq((const Seqable*)obj);
	for(const ISeq *s = (const ISeq*)is; s != NULL; s = s->obj.fns->ISeqFns->next(s), ms = ms->obj.fns->ISeqFns->next(ms)) {
		if(ms == NULL) return false;
		if(!Equiv(s->obj.fns->ISeqFns->first(s), ms->obj.fns->ISeqFns->first(ms)))
			return false;
	}
	return ms == NULL;
}

const ISeq* seqASeq(const Seqable *s) {
	assert(isISeq(&s->obj));
	return (ISeq*) s;
}
