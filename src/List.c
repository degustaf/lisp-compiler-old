#include "List.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ASeq.h"
#include "Interfaces.h"
#include "Util.h"

static LLVMValueRef List_codegen(const struct lisp_object_struct *obj);
static const char *toStringEmptyList(const lisp_object *obj);
static const lisp_object* firstList(const ISeq*);
static const ISeq* nextList(const ISeq*);
static const lisp_object *ListCopy(const lisp_object *obj);

Seqable_vtable List_Seqable_vtable = {
	seqASeq // seq
};

ICollection_vtable List_ICollection_vtable = {
	NULL, // count	// TODO
	emptyASeq, // empty
	EquivASeq  // Equiv
};

ISeq_vtable List_ISeq_vtable = {
	firstList, // first
	nextList, // next
	moreASeq, // more
	NULL, // cons	// TODO
};

interfaces List_interfaces = {
	&List_Seqable_vtable, // Seqable_vtable
	&List_ICollection_vtable,	//ICollection_vtable
	&List_ISeq_vtable, // ISeq_vtable
	NULL
};

struct List_struct {
    lisp_object obj;
    const lisp_object *const _first;
    const struct List_struct *const _rest;
    const size_t _count;
};
    
const List _EmptyList = {{LIST_type, List_codegen, toStringEmptyList, ListCopy, &List_interfaces}, NULL, NULL, 0};
const List *const EmptyList = &_EmptyList;

static LLVMValueRef List_codegen(__attribute__((unused)) const struct lisp_object_struct *obj) {
    return NULL;
}

static const char *toStringEmptyList(const lisp_object *obj) {
	assert((void*)obj == (void*)EmptyList);
	return "()";
}

static const lisp_object *ListCopy(const lisp_object *obj) {
    assert(obj->type == LIST_type);
    List *list = (List*)obj;
    size_t count = list->_count;
    const lisp_object **arr = malloc(count * sizeof(*arr));
    for(size_t i=0; i<count; i++) {
        const lisp_object *const o = list->_first;
        list = (List*)list->_rest;
        arr[i] = (*(o->copy))((lisp_object*)o);
    }
    return (lisp_object*)CreateList(count, arr);
}

const List *NewList(const lisp_object *const first) {
    List *ret = malloc(sizeof(*ret));
    if(ret == NULL) return NULL;

    List _ret = {{LIST_type, List_codegen, toString, ListCopy, &List_interfaces}, first, EmptyList, 1};
    memcpy(ret, &_ret, sizeof(*ret));

    return ret;
}

const List *CreateList(size_t count, const lisp_object **entries) {
    List *ret = (List*)EmptyList;
    for(size_t i = 1; i <= count; i++) {
        List _ret = {{LIST_type, List_codegen, toString, ListCopy, &List_interfaces},
                     entries[count-i],
                     ret,
                     i};
        ret = malloc(sizeof(*ret));
        memcpy(ret, &_ret, sizeof(*ret));
    }
    return ret;
}

static const lisp_object* firstList(const ISeq *is) {
	assert(is->obj.type == LIST_type);
	const List *l = (List*) is;

	return l->_first;
}

static const ISeq* nextList(const ISeq *is) {
	assert(is->obj.type == LIST_type);
	const List *l = (List*) is;

	if(l->_count == 1) return NULL;
	return (ISeq*) l->_rest;
}
