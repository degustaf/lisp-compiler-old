#include "Vector.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "AFn.h"
#include "ASeq.h"
#include "AVector.h"
#include "Error.h"
#include "gc.h"
#include "Interfaces.h"
#include "lisp_pthread.h"
#include "Map.h"
#include "Util.h"

#define LOG_NODE_SIZE 5
#define NODE_SIZE (1 << LOG_NODE_SIZE)
#define BITMASK (NODE_SIZE - 1)

// Node

typedef struct {
    lisp_object obj;
    bool editable;
    pthread_t thread_id;
    const lisp_object *array[NODE_SIZE];
} Node;

// Node Function Declarations.

static Node *NewNode(bool editable, pthread_t thread_id, size_t count, const lisp_object* const* array);
static Node *editableRoot(const Node *const root);
static Node *newPath(bool editable, pthread_t thread_id, size_t level, Node *node);

const Node _EmptyNode = {{NODE_type, sizeof(Node), NULL, NULL, NULL, NULL}, false, 0, {NULL}};
const Node *const EmptyNode = &_EmptyNode;

// ChunkedSeq

typedef struct {
	lisp_object obj;
	const Vector *vec;
	size_t count;
	const lisp_object *node[NODE_SIZE];
	size_t i;
	size_t offset;
} ChunkedSeq;

// ChunkedSeq Function Declarations.

static ChunkedSeq* NewChunkedSeq(const IMap *meta, const Vector *v, size_t count, const lisp_object* const* node, size_t i, size_t offset);
static size_t countChunkedSeq(const ICollection*);
static const lisp_object* firstChunkedSeq(const ISeq*);
static const ISeq* nextChunkedSeq(const ISeq*);
static const ISeq* chunkedNextChunkedSeq(const ChunkedSeq*);

const Seqable_vtable ChunkedSeq_Seqable_vtable = {
	seqASeq,			// seq
};

const ICollection_vtable ChunkedSeq_ICollection_vtable = {
	countChunkedSeq,			// count
	(ICollectionFn1)consASeq,	// cons
	emptyASeq,					// empty
	EquivASeq					// Equiv
};

const ISeq_vtable ChunkedSeq_ISeq_vtable = {
	firstChunkedSeq,	// first
	nextChunkedSeq,		// next
	moreASeq,			// more
	consASeq,			// cons
};

interfaces ChunkSeq_interfaces = {
	&ChunkedSeq_Seqable_vtable,		// SeqableFns
	NULL,							// ReversibleFns
	&ChunkedSeq_ICollection_vtable,	// ICollectionFns
	NULL,							// IStackFns
	&ChunkedSeq_ISeq_vtable,		// ISeqFns
	NULL,							// IFnFns
	NULL,							// IVectorFns
	NULL,							// IMapFns
};


// TransientVector

typedef struct {
    lisp_object obj;
    size_t count;
    size_t shift;
    Node *root;
    const lisp_object *tail[NODE_SIZE];
} TransientVector;

// Vector

struct Vector_struct {
    lisp_object obj;
    const size_t count;
    const size_t shift;
    const Node *const root;
    const lisp_object *tail[NODE_SIZE];
};

// TransientVector Function Declarations.

static TransientVector *TransVecConj(TransientVector *v, const lisp_object *obj);
static size_t tailoffTV(TransientVector *v);
static Node *pushTailTV(TransientVector *v, size_t level, const Node *parent, Node *tail);
static Node *ensureEditable(TransientVector *v, const Node *node);
static const Vector *asPersistent(TransientVector *v);

// Vector Function Declarations.

static const Vector *NewVector(size_t cnt, size_t shift, const Node *root, size_t count, const lisp_object* const* array);
static TransientVector *asTransient(const Vector *v);
static const ISeq* seqVector(const Seqable*);
static size_t countVector(const ICollection *o);
static const IStack* popVector(const IStack*);
static const IVector* assocNVector(const IVector*, size_t, const lisp_object*);
static const IVector* consVector(const IVector*, const lisp_object*);
static const lisp_object *nthVector(const IVector *iv, size_t n, const lisp_object *notFound);
static const lisp_object *const *arrayForV(const Vector *v, size_t i);
static size_t tailoffV(const Vector *v);
static Node *pushTailV(const Vector *v, size_t level, const Node *parent, Node *tail);

const Seqable_vtable Vector_Seqable_vtable = {
	seqVector, // seq
};

const Reversible_vtable Vector_Reversible_vtable = {
	rseq // rseq
};

const ICollection_vtable Vector_ICollection_vtable = {
	countVector,					// count
	(ICollectionFn1) consVector,	// cons
	emptyAVector,					// empty
	EquivAVector,					// Equiv
};

const IStack_vtable Vector_IStack_vtable = {
	peekAVector,	// peek
	popVector,		// pop
};

const IFn_vtable Vector_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1AVector,	// invoke1
	invoke2AFn,		// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
};

const IVector_vtable Vector_IVector_vtable = {
	lengthAVector,	// length
	assocNVector,	// assocN
	consVector,		// cons
	assocAVector,	// assoc
	entryAtAVector,	// entryAt
	nthVector,		// nth
};

const interfaces Vector_interfaces = {
	&Vector_Seqable_vtable,		// SeqableFns
	&Vector_Reversible_vtable,	// ReversibleFns
	&Vector_ICollection_vtable,	// ICollectionFns
	&Vector_IStack_vtable,		// IStackFns
	NULL,						// ISeqFns
	&Vector_IFn_vtable,			// IFnFns
	&Vector_IVector_vtable,		// IVectorFns
	NULL,						// IMapFns
};

const Vector _EmptyVector = {{VECTOR_type, sizeof(Vector), toString, EqualsAVector, NULL, &Vector_interfaces},
                   0,
                   LOG_NODE_SIZE,
                   &_EmptyNode,
                   {NULL}};
const Vector *const EmptyVector = &_EmptyVector;

// Node Function Definitions.

Node *NewNode(bool editable, pthread_t thread_id, size_t count, const lisp_object* const* array) {
	Node *ret = GC_MALLOC(sizeof(*ret));
	memcpy(ret, EmptyNode, sizeof(*ret));
	ret->editable = editable;
	ret->thread_id = thread_id;
	if(count > 0)
		memcpy(ret->array, array, count * sizeof(*(ret->array)));

	return ret;
}

Node *editableRoot(const Node *const root) {
    Node *ret = GC_MALLOC(sizeof(*ret));
    memset(ret, '\0', sizeof(*ret));
    ret->obj.type = NODE_type;
    ret->editable = true;
    ret->thread_id = pthread_self();

    for(size_t i = 0; i < NODE_SIZE; i++) {
        const lisp_object *o = root->array[i];
        if(o == NULL)
            break;
        ret->array[i] = copy(o);
    }

    return ret;
}

static Node *newPath(bool editable, pthread_t thread_id, size_t level, Node *node) {
	if(level == 0) return node;
	Node *ret = NewNode(editable, thread_id, 0, NULL);
	ret->array[0] = (lisp_object*) newPath(editable, thread_id, level - LOG_NODE_SIZE, node);
	return ret;
}

// ChunkedSeq Function Declarations.

static ChunkedSeq* NewChunkedSeq(const IMap *meta, const Vector *v, size_t count, const lisp_object* const* node, size_t i, size_t offset) {
	ChunkedSeq *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = CHUNKEDSEQ_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.toString = toString;
	ret->obj.Equals = EqualsASeq;
	ret->obj.meta = meta ? meta : (IMap*) EmptyHashMap;
	ret->obj.fns = &ChunkSeq_interfaces;

	ret->vec = v;
	ret->i = i;
	ret->offset = offset;
	if(node) {
		memcpy(ret->node, node, count);
		ret->count = count;
	} else {
		memcpy(ret->node, arrayForV(v, i), NODE_SIZE);
		ret->count = NODE_SIZE;
	}

	return ret;
}

static size_t countChunkedSeq(const ICollection *ic) {
	assert(ic->obj.type == CHUNKEDSEQ_type);
	const ChunkedSeq *cs = (ChunkedSeq*)ic;
	return cs->vec->count - (cs->i + cs->offset);
}

static const lisp_object* firstChunkedSeq(const ISeq *is) {
	assert(is->obj.type == CHUNKEDSEQ_type);
	const ChunkedSeq *cs = (ChunkedSeq*)is;
	return cs->node[cs->offset];
}

static const ISeq* nextChunkedSeq(const ISeq *is) {
	assert(is->obj.type == CHUNKEDSEQ_type);
	const ChunkedSeq *cs = (ChunkedSeq*)is;
	if(cs->offset + 1 < cs->count)
		return (ISeq*) NewChunkedSeq(NULL, cs->vec, cs->count, cs->node, cs->i, cs->offset+1);
	return chunkedNextChunkedSeq(cs);
}

static const ISeq* chunkedNextChunkedSeq(const ChunkedSeq *cs) {
	if(cs->i + cs->count < cs->vec->count) {
		return (ISeq*) NewChunkedSeq(NULL, cs->vec, 0, NULL, cs->i + cs->count, 0);
	}
	return NULL;
}

// TransientVector Function Definitions.

static TransientVector *TransVecConj(TransientVector *v, const lisp_object *obj) {
    assert(v->root->editable);
    size_t i = v->count;
    if(i-tailoffTV(v) < NODE_SIZE) {
        v->tail[i & BITMASK] = obj;
        v->count++;
        return v;
    }
	int newshift = v->shift;
	Node *newroot = NULL;
	Node *tailnode = NewNode(v->root->editable, v->root->thread_id, i, v->tail);
	memset(v->tail, '0', NODE_SIZE * sizeof(*(v->tail)));
	v->tail[0] = obj;
	if((v->count >> LOG_NODE_SIZE) > (((size_t)1) << v->shift)) {
		newroot = NewNode(v->root->editable, v->root->thread_id, 0, NULL);
		newroot->array[0] = (const lisp_object*) v->root;
		newroot->array[1] = (lisp_object*) newPath(true, v->root->thread_id, newshift, tailnode);
		newshift += LOG_NODE_SIZE;
	} else {
		newroot = pushTailTV(v, newshift, v->root, tailnode);
	}
	v->root = newroot;
	v->shift = newshift;
	v->count++;
	return v;
}

static size_t tailoffTV(TransientVector *v) {
    if(v->count <= 32)
        return 0;
    return (v->count - 1) & ~BITMASK;
}

static Node *pushTailTV(TransientVector *v, size_t level, const Node *parent, Node *tail) {
	Node *ret = ensureEditable(v, parent);
	size_t idx = ((v->count - 1) >> level) & BITMASK;
	Node *node_to_insert = NULL;
	if(level == 5) {
		node_to_insert = tail;
	} else {
		Node *child = (Node*) parent->array[idx];
		if(child == NULL) {
			node_to_insert = newPath(true, v->root->thread_id, level - LOG_NODE_SIZE, tail);
		} else {
			node_to_insert = pushTailTV(v, level - LOG_NODE_SIZE, child, tail);
		}
	}
	ret->array[idx] = (lisp_object*) node_to_insert;
	return ret;
}

static Node *ensureEditable(TransientVector *v, const Node *node) {
	assert(v->root->editable);
	if(node->editable && (v->root->thread_id == node->thread_id))
		return (Node*) node;
	return NewNode(true, v->root->thread_id, NODE_SIZE, node->array);
}

static const Vector *asPersistent(TransientVector *v) {
	assert(v->root->editable);
	v->root->editable = false;
	size_t new_cnt = v->count - tailoffTV(v);
	return NewVector(v->count, v->shift, v->root, new_cnt, v->tail);
}

// Vector Function Definitions.

static const Vector *NewVector(size_t cnt, size_t shift, const Node *root, size_t count, const lisp_object* const* array) {
    Vector *ret = GC_MALLOC(sizeof(*ret));
	assert(ret);
    memcpy(ret, EmptyVector, sizeof(*ret));
	memcpy((void*) &ret->count, &cnt, sizeof(cnt));
	memcpy((void*) &ret->shift, &shift, sizeof(shift));
	memcpy((void*) &ret->root, &root, sizeof(root));
	memcpy(ret->tail, array, count * sizeof(*array));

	return ret;
}

const Vector *CreateVector(size_t count, const lisp_object *const *entries) {
	if(count == 0) return EmptyVector;
    Vector *ret = GC_MALLOC(sizeof(*ret));
    size_t cnt = count < NODE_SIZE ? count : NODE_SIZE;
    memcpy(ret, EmptyVector, sizeof(*ret));
	memcpy((void*) &ret->count, &cnt, sizeof(cnt));

    for(size_t i = 0; i < cnt; i++) {
        ret->tail[i] = entries[i];
    }

    if(count <= NODE_SIZE)
        return ret;
    TransientVector *trans = asTransient(ret);
    for(size_t i = NODE_SIZE; i < count; i++) {
		trans = TransVecConj(trans, entries[i]);
    }

	return asPersistent(trans);
}

static TransientVector *asTransient(const Vector *v) {
    assert(sizeof(TransientVector) == sizeof(Vector));
    TransientVector *ret = GC_MALLOC(sizeof(*ret));

    memset(ret, '\0', sizeof(*ret));
    memcpy(&ret->obj, &v->obj, sizeof(ret->obj));
    ret->count = v->count;
    ret->shift = v->shift;
    ret->root = editableRoot(v->root);
    for(size_t i = 0; i <NODE_SIZE; i++) {
        const lisp_object *o = v->tail[i];
        if(o == NULL)
            break;
        ret->tail[i] = copy(o);
    }

    return ret;
}

static const ISeq* seqVector(const Seqable *self) {
	assert(self->obj.type == VECTOR_type);
	const Vector *v = (const Vector*) self;

	if(v->count == 0)
		return NULL;
	return (ISeq*)NewChunkedSeq(NULL, v, 0, NULL, 0, 0);
}

static size_t countVector(const ICollection *o) {
	assert(o->obj.type == VECTOR_type);
	const Vector *v = (const Vector*) o;
	return v->count;
}

static const IVector* consVector(const IVector *iv, const lisp_object *x) {
	assert(iv->obj.type == VECTOR_type);
	const Vector *v = (const Vector*) iv;
	if(v->count - tailoffV(v) < NODE_SIZE) {
		const lisp_object *newTail[NODE_SIZE];
		memcpy(newTail, v->tail, (v->count & BITMASK) * sizeof(newTail[0]));
		newTail[v->count] = x;
		return (IVector*)NewVector(v->count + 1, v->shift, v->root, v->count & BITMASK, newTail);
	}
	Node *NewRoot = NULL;
	Node *TailNode = NewNode(v->root->editable, v->root->thread_id, v->count & BITMASK, v->tail);
	size_t newshift = v->shift;
	if((v->count >> LOG_NODE_SIZE) > ((size_t)1 << v->shift)) {
		NewRoot = NewNode(v->root->editable, v->root->thread_id, 0, NULL);
		NewRoot->array[0] = (lisp_object*)v->root;
		NewRoot->array[1] = (lisp_object*)newPath(v->root->editable, v->root->thread_id, v->shift, TailNode);
		newshift += LOG_NODE_SIZE;
	} else {
		NewRoot = pushTailV(v, v->shift, v->root, TailNode);
	}
	const lisp_object *NewTail[NODE_SIZE];
	NewTail[0] = x;
	return (IVector*)NewVector(v->count + 1, newshift, NewRoot, 1, NewTail);
}

static const Node* popTail(const Vector *v, size_t shift, const Node *node) {
	size_t subindex = ((v->count - 2) >> shift) & BITMASK;
	if(shift > LOG_NODE_SIZE) {
		const Node *newChild = popTail(v, shift - LOG_NODE_SIZE, (Node*) node->array[subindex]);
		if(newChild == NULL && subindex == 0)
			return NULL;
		Node *ret = NewNode(v->root->editable, v->root->thread_id, NODE_SIZE, node->array);
		ret->array[subindex] = (lisp_object*)newChild;
		return ret;
	}

	if(subindex == 0)
		return NULL;

	Node *ret = NewNode(v->root->editable, v->root->thread_id, NODE_SIZE, node->array);
	ret->array[subindex] = NULL;
	return ret;
}

static const IStack* popVector(const IStack *is) {
	assert(is->obj.type == VECTOR_type);
	const Vector *v = (Vector*)is;
	if(v->count == 0) {
		exception e = {IllegalStateException, "Can't pop empty vector"};
		Raise(e);
	}
	if(v->count == 1)
		return (IStack*)withMeta((lisp_object*)EmptyVector, v->obj.meta);

	if(v->count  - tailoffV(v) > 1) {
		const lisp_object *newTail[NODE_SIZE];
		memcpy(newTail, v->tail, sizeof(newTail));
		return (IStack*)NewVector(v->count - 1, v->shift, v->root, NODE_SIZE, newTail);
	}

	const lisp_object * const* newTail = arrayForV(v, v->count -2);
	const Node *newRoot = popTail(v, v->shift, v->root);
	size_t newshift = v->shift;
	if(newRoot == NULL) {
		newRoot = EmptyNode;
	} else {
		newRoot = (Node*)newRoot->array[0];
		newshift -= LOG_NODE_SIZE;
	}

	return (IStack*) NewVector(v->count - 1, newshift, newRoot, NODE_SIZE, newTail);
}

static const Node* doAssoc(size_t shift, const Node *node, size_t n, const lisp_object *val) {
	Node *ret = NewNode(node->editable, node->thread_id, NODE_SIZE, node->array);

	if(shift == 0) {
		ret->array[n & BITMASK] = val;
	} else {
		size_t subindex = (n >> shift) & BITMASK;
		ret->array[subindex] = (lisp_object*) doAssoc(shift - LOG_NODE_SIZE, (Node*) node->array[subindex], n, val);
	}
	
	return ret;
}

static const IVector* assocNVector(const IVector *iv, size_t n, const lisp_object *val) {
	assert(iv->obj.type == VECTOR_type);
	const Vector *v = (Vector*)iv;
	if(n < v->count) {
		if(n >= tailoffV(v)) {
			const lisp_object *newTail[NODE_SIZE];
			memcpy(newTail, v->tail, sizeof(newTail));
			newTail[n & BITMASK] = val;
			return (IVector*) NewVector(v->count, v->shift, v->root, v->count & BITMASK, newTail);
		}
		return (IVector*) NewVector(v->count, v->shift, doAssoc(v->shift, v->root, n, val), v->count & BITMASK, v->tail);
	}

	if(n == v->count)
		return consVector(iv, val);
	exception e = {IndexOutOfBoundException, ""};
	Raise(e);
	__builtin_unreachable();
}

static const lisp_object *nthVector(const IVector *iv, size_t n, const lisp_object *notFound) {
	assert(iv->obj.type == VECTOR_type);
	const Vector *v = (const Vector*) iv;
	if(n < v->count) {
		const lisp_object *const *node = arrayForV(v, n);
		return node[n & BITMASK];
	}
	return notFound;
}

static const lisp_object *const *arrayForV(const Vector *v, size_t i) {
	assert(i < v->count);
	if(i >= tailoffV(v))
		return v->tail;
	const Node *node = v->root;
	assert(v->shift % 5 == 0);
	for(size_t level = v->shift; level > 0; level -= LOG_NODE_SIZE) {
		node = (const Node*) node->array[(i >> level) & BITMASK];
	}
	return node->array;
}

static size_t tailoffV(const Vector *v) {
    if(v->count <= 32)
        return 0;
    return (v->count - 1) & ~BITMASK;
}

static Node *pushTailV(const Vector *v, size_t level, const Node *parent, Node *tail) {
	size_t idx = ((v->count) >> level) & BITMASK;
	Node *ret = NewNode(parent->editable, parent->thread_id, v->count & BITMASK, parent->array);
	Node *NodeToInsert = NULL;
	if(level == LOG_NODE_SIZE) {
		NodeToInsert = tail;
	} else {
		Node *child = (Node*)parent->array[idx];
		NodeToInsert = child ? newPath(v->root->editable, v->root->thread_id, level - LOG_NODE_SIZE, tail)
							 : pushTailV(v, level - LOG_NODE_SIZE, child, tail);
	}
	ret->array[idx] = (lisp_object*)NodeToInsert;
	return ret;
}

int indexOf(const IVector *iv, const lisp_object *o) {
	for(size_t i = 0; i< iv->obj.fns->ICollectionFns->count((ICollection*)iv); i++) {
		if(Equiv(o, iv->obj.fns->IVectorFns->nth(iv, i, NULL)))
			return i;
	}
	return -1;
}
