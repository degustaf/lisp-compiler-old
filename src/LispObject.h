#ifndef LISP_OBJECT_H
#define LISP_OBJECT_H

#include <stdbool.h>
#include <stddef.h>

// #include "llvm-c/Types.h"

// These can be extracted out of this file to a seperate header if needed for another enum.
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(ENUM) #ENUM,

#define FOREACH_TYPE(TYPE) \
	TYPE(LISPOBJECT_type) \
	TYPE(EOF_type) \
	TYPE(ERROR_type) \
    TYPE(DONE_type) \
    TYPE(NOOP_type) \
	TYPE(BOOL_type) \
    TYPE(CHAR_type) \
    TYPE(STRING_type) \
    TYPE(INTEGER_type) \
    TYPE(FLOAT_type) \
    TYPE(LIST_type) \
	TYPE(CONS_type) \
	TYPE(SYMBOL_type) \
	TYPE(KEYWORD_type) \
	TYPE(NAMESPACE_type) \
	TYPE(UNBOUND_type) \
	TYPE(VAR_type) \
	TYPE(IFN_type) \
	TYPE(EXPR_type) \
	TYPE(PATHNODE_type) \
	TYPE(OBJMETHOD_type) \
	TYPE(FNMETHOD_type) \
	TYPE(BINDINGINIT_type) \
\
	/* Map types. */ \
	TYPE(MAPENTRY_type) \
	TYPE(BMI_NODE_type) \
	TYPE(ARRAY_NODE_type) \
	TYPE(COLLISIONNODE_type) \
	TYPE(NODESEQ_type) \
	TYPE(ARRAYNODESEQ_type) \
	TYPE(HASHMAP_type) \
	TYPE(TRANSIENTHASHMAP_type) \
	TYPE(KEYSEQ_type) \
\
	/* Vector types. */ \
	TYPE(VECTOR_type) \
	TYPE(NODE_type) \
	TYPE(CHUNKEDSEQ_type) \
	TYPE(RSEQ_type) \
\
	/* Interfaces. */ \
	TYPE(ISEQ_interface)


typedef enum {
	FOREACH_TYPE(GENERATE_ENUM)
} object_type;

static const char *object_type_string[] __attribute__((unused)) = {
	FOREACH_TYPE(GENERATE_STRING)
} ;

typedef struct Seqable_vtable_struct Seqable_vtable;
typedef struct Reversible_vtable_struct Reversible_vtable;
typedef struct ICollection_vtable_struct ICollection_vtable;
typedef struct IStack_vtable_struct IStack_vtable;
typedef struct ISeq_vtable_struct ISeq_vtable;
typedef struct IFn_vtable_struct IFn_vtable;
typedef struct IVector_vtable_struct IVector_vtable;
typedef struct IMap_vtable_struct IMap_vtable;

typedef struct {
	const Seqable_vtable *SeqableFns;
	const Reversible_vtable *ReversibleFns;
	const ICollection_vtable *ICollectionFns;
	const IStack_vtable *IStackFns;
	const ISeq_vtable *ISeqFns;
	const IFn_vtable *IFnFns;
	const IVector_vtable *IVectorFns;
	const IMap_vtable *IMapFns;
} interfaces;

static const interfaces NullInterface = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

typedef struct IMap_struct IMap;

typedef struct lisp_object_struct {
    object_type type;
	size_t size;
	const char *(*toString)(const struct lisp_object_struct *);
	bool (*Equals)(const struct lisp_object_struct *x, const struct lisp_object_struct *y);
	const IMap *meta;
	const interfaces *fns;
} lisp_object;

#endif /* LISP_OBJECT_H */
