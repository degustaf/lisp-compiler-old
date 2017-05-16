#ifndef LISP_OBJECT_H
#define LISP_OBJECT_H

#include "stddef.h"

#include "llvm-c/Types.h"

// These can be extracted out of this file to a seperate hesader if needed for another enum.
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(ENUM) #ENUM,

#define FOREACH_TYPE(TYPE) \
	TYPE(EOF_type) \
	TYPE(ERROR_type) \
    TYPE(DONE_type) \
    TYPE(NOOP_type) \
    TYPE(CHAR_type) \
    TYPE(STRING_type) \
    TYPE(INTEGER_type) \
    TYPE(FLOAT_type) \
    TYPE(LIST_type) \
	TYPE(CONS_type) \
\
	/* Map types. */ \
	TYPE(BMI_NODE_type) \
	TYPE(ARRAY_NODE_type) \
	TYPE(NODESEQ_type) \
	TYPE(HASHMAP_type) \
	TYPE(TRANSIENTHASHMAP_type) \
\
	/* Vector types. */\
	TYPE(VECTOR_type) \
	TYPE(NODE_type)


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
typedef struct IVector_vtable_struct IVector_vtable;
typedef struct IMap_vtable_struct IMap_vtable;

typedef struct {
	Seqable_vtable *SeqableFns;
	Reversible_vtable *ReversibleFns;
	ICollection_vtable *ICollectionFns;
	IStack_vtable *IStackFns;
	ISeq_vtable *ISeqFns;
	IVector_vtable *IVectorFns;
	IMap_vtable *IMapFns;
} interfaces;

static const interfaces NullInterface = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

typedef struct lisp_object_struct {
    object_type type;
    LLVMValueRef (*codegen)(const struct lisp_object_struct *);
	const char *(*toString)(const struct lisp_object_struct *);
    const struct lisp_object_struct* (*copy)(const struct lisp_object_struct *);
	const interfaces *fns;
} lisp_object;

#endif /* LISP_OBJECT_H */
