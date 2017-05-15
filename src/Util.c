#include "Util.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "Interfaces.h"
#include "List.h"
#include "Map.h"
#include "Numbers.h"
#include "Murmur3.h"

bool Equiv(const lisp_object *x, const lisp_object *y) {
	if(x == y)
		return true;
	if(x) {
		// TODO	Extend Equiv
		// If x and y are Numbers.
			// return Numbers.Equal(x, y)
		// If x is PersistentCololection
			// return x.equiv(y)
		// If y is PersistentCollection
			// return y.equiv(x)
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
		case LIST_type:		// TODO HashEq
		case CONS_type:		// TODO HashEq
		case NODESEQ_type:	// TODO HashEq
		case HASHMAP_type:	// TODO HashEq
		default:
			return hash32(&x, sizeof(x));
	}
}

static void PrintObject(StringWriter *sw, const lisp_object *obj) {
	printf("In PrintObject\n");
	fflush(stdout);
	assert(sw);
	if((void*) obj == (void*) EmptyList) {
		AddString(sw, "()");
		return;
	}
	printf("Not EmptyList\n");
	fflush(stdout);
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
	printf("Not List\n");
	fflush(stdout);
	if(isIMap(obj)) {
		printf("In map section.\n");
		fflush(stdout);
		if(obj->fns->ICollectionFns->count((const ICollection*)obj) == 0) {
			printf("In empty map section.\n");
			fflush(stdout);
			AddString(sw, "{}");
			return;
		}

		printf("After empty map section.\n");
		fflush(stdout);

		AddChar(sw, '{');

		const ISeq *s = obj->fns->SeqableFns->seq((Seqable*)obj);
		printf("got s\n");
		fflush(stdout);
		for(; s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
			printf("In HashMaptoString for loop.\n");
			fflush(stdout);
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
	printf("Not Map\n");
	fflush(stdout);
	AddString(sw, obj->toString(obj));
}

const char *toString(const lisp_object *obj) {
	printf("In toString\n");
	fflush(stdout);
	StringWriter *sw = NewStringWriter();
	PrintObject(sw, obj);
	return WriteString(sw);
}
