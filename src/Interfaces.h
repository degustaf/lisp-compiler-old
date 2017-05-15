#ifndef INTERFACES_H
#define INTERFACES_H

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "LispObject.h"

// Interfaces

typedef struct {	// Seqable
	const lisp_object obj;
} Seqable;

typedef struct {	// ICollection
	const lisp_object obj;
} ICollection;

typedef struct {	// ISeq
	const lisp_object obj;
} ISeq;

typedef struct {	// IMap
	const lisp_object obj;
} IMap;

struct Seqable_vtable_struct {
	const ISeq* (*seq)(const Seqable*);
};

struct ICollection_vtable_struct {
	size_t (*count)(const ICollection*);
	const ICollection* (*empty)(void);
	bool (*Equiv)(const ICollection*, const lisp_object*);
};

struct ISeq_vtable_struct {
	const lisp_object* (*first)(const ISeq*);
	const ISeq* (*next)(const ISeq*);
	const ISeq* (*more)(const ISeq*);
	const ISeq* (*cons)(const ISeq*, const lisp_object*);
};

struct IMap_vtable_struct {
	const IMap* (*assoc)(const IMap*, const lisp_object*, const lisp_object*);
	const IMap* (*without)(const IMap*, const lisp_object*);
	size_t (*count)(const IMap*);
	const lisp_object* (*entryAt)(const IMap*, const lisp_object*);
	const IMap* (*cons)(const IMap*, const lisp_object*);
};

// Instance functions.
// These functions are designed to check if a lisp_object satisfies an iterface.
// They are here so that they can be inlined by the compiler.
// They also use assert to verify Interface Inheritance.

static inline bool isSeqable(const lisp_object *obj) {
	return (bool)obj->fns->SeqableFns;
}

static inline bool isICollection(const lisp_object *obj) {
	bool ret = (bool)obj->fns->ICollectionFns;
	if(ret) {
		assert(isSeqable(obj));
	}
	return ret;
}

static inline bool isISeq(const lisp_object *obj) {
	printf("obj = %p\n", (void*)obj);
	fflush(stdout);
	printf("obj->fns = %p\n", (void*) obj->fns);
	fflush(stdout);
	bool ret = (bool)obj->fns->ISeqFns;
	if(ret) {
		assert(isICollection(obj));
	}
	return ret;
}

static inline bool isIMap(const lisp_object *obj) {
	bool ret = (bool)obj->fns->IMapFns;
	if(ret) {
		assert(isICollection(obj));
	}
	return ret;
}

#endif /* INTERFACES_H */
