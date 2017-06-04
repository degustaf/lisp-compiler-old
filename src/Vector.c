#include "Vector.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "AFn.h"
#include "gc.h"
#include "Interfaces.h"
#include "lisp_pthread.h"
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

const Node _EmptyNode = {{NODE_type, sizeof(Node), NULL, NULL, NULL, NULL, NULL}, false, 0, {NULL}};
const Node *const EmptyNode = &_EmptyNode;

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
static const lisp_object *VectorCopy(const lisp_object *obj);
static TransientVector *asTransient(const Vector *v);
static size_t countVector(const ICollection *o);
static const lisp_object *nthVector(const IVector *iv, size_t n, const lisp_object *notFound);
static const lisp_object *const *arrayForV(const Vector *v, size_t i);
static size_t tailoffV(const Vector *v);

Seqable_vtable Vector_Seqable_vtable = {
	NULL // seq		// TODO
};

Reversible_vtable Vector_Reversible_vtable = {
	NULL // rseq		// TODO
};

ICollection_vtable Vector_ICollection_vtable = {
	countVector,	// count
	NULL,	// empty	// TODO
	NULL,	//Equiv		// TODO
};

IStack_vtable Vector_IStack_vtable = {
	NULL,	// peek	// TODO
	NULL,	// pop	// TODO
};

IFn_vtable Vector_IFn_vtable = {
	invoke0AFn,	// invoke0
	NULL,		// invoke1	// TODO
	invoke2AFn,	// invoke2
	invoke3AFn,	// invoke3
	invoke4AFn,	// invoke4
	invoke5AFn,	// invoke5
	NULL,		// applyTo	// TODO
};

IVector_vtable Vector_IVector_vtable = {
	NULL,	// length	// TODO
	NULL,	// assocN	// TODO
	NULL,	// cons		// TODO
	NULL,	// assoc	// TODO
	NULL,	// entryAt	// TODO
	nthVector,	// nth
};

interfaces Vector_interfaces = {
	&Vector_Seqable_vtable,		// SeqableFns
	&Vector_Reversible_vtable,	// ReversibleFns
	&Vector_ICollection_vtable,	// ICollectionFns
	&Vector_IStack_vtable,		// IStackFns
	NULL,						// ISeqFns
	&Vector_IFn_vtable,			// IFnFns
	&Vector_IVector_vtable,		// IVectorFns
	NULL,						// IMapFns
};

const Vector _EmptyVector = {{VECTOR_type, sizeof(Vector), toString, VectorCopy, EqualBase, NULL, &Vector_interfaces},
	// TODO EqualVector
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
        ret->array[i] = (*(o->copy))(o);
    }

    return ret;
}

static Node *newPath(bool editable, pthread_t thread_id, size_t level, Node *node) {
	if(level == 0) return node;
	Node *ret = NewNode(editable, thread_id, 0, NULL);
	ret->array[0] = (lisp_object*) newPath(editable, thread_id, level - LOG_NODE_SIZE, node);
	return ret;
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

const Vector *CreateVector(size_t count, const lisp_object **entries) {
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

static const lisp_object *VectorCopy(__attribute__((unused)) const lisp_object *obj){
    // TODO		// VectorCopy
    return NULL;
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
        ret->tail[i] = o->copy(o);
    }

    return ret;
}

static size_t countVector(const ICollection *o) {
	assert(o->obj.type == VECTOR_type);
	const Vector *v = (const Vector*) o;
	return v->count;
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
