#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "Vector.h"

#define LOG_NODE_SIZE 5
#define NODE_SIZE (1 << LOG_NODE_SIZE)
#define BITMASK (NODE_SIZE - 1)

typedef struct {
    lisp_object obj;
    bool editable;
    pthread_t thread_id;
    lisp_object *array[NODE_SIZE];
} Node;

const Node _EmptyNode = {{NODE_TYPE, NULL, NULL, NULL}, false, 0, {NULL}};
const Node *const EmptyNode = &_EmptyNode;

Node *NewNode(bool editable, pthread_t thread_id, size_t count, lisp_object **array) {
	Node *ret = malloc(sizeof(*ret));
	memcpy(ret, EmptyNode, sizeof(*ret));
	ret->editable = editable;
	ret->thread_id = thread_id;
	memcpy(ret->array, array, count * sizeof(*(ret->array)));

	return ret;
}

Node *editableRoot(const Node *const root) {
    Node *ret = malloc(sizeof(*ret));
    memset(ret, '\0', sizeof(*ret));
    ret->obj.type = NODE_TYPE;
    ret->editable = true;
    ret->thread_id = pthread_self();

    for(size_t i = 0; i < NODE_SIZE; i++) {
        lisp_object *o = root->array[i];
        if(o == NULL)
            break;
        ret->array[i] = (*(o->copy))(o);
    }

    return ret;
}


struct Vector_struct {
    lisp_object obj;
    const size_t count;
    const size_t shift;
    const Node *const root;
    lisp_object *tail[NODE_SIZE];
};

typedef struct {
    lisp_object obj;
    size_t count;
    size_t shift;
    Node *root;
    lisp_object *tail[NODE_SIZE];
} TransientVector;

const Vector *const EmptyVector;    // TODO

LLVMValueRef VectorCodegen(lisp_object *obj);
char *VectorToString(lisp_object *obj);
lisp_object *VectorCopy(lisp_object *obj);
static TransientVector *asTransient(Vector *v);
static TransientVector *TransVecConj(TransientVector *v, lisp_object *obj);

const Vector *CreateVector(size_t count, lisp_object **entries) {
    Vector *ret = malloc(sizeof(*ret));
    size_t cnt = count < NODE_SIZE ? count : NODE_SIZE;
    Vector _ret = {{VECTOR_TYPE, &VectorCodegen, &VectorToString, VectorCopy},
                   cnt,
                   LOG_NODE_SIZE,
                   EmptyNode,
                   {NULL}};
    memcpy(ret, &_ret, sizeof(*ret));

    for(size_t i = 0; i < cnt; i++) {
        ret->tail[i] = entries[i];
    }

    if(count <= NODE_SIZE)
        return ret;
    TransientVector *trans = asTransient(ret);
    for(size_t i = NODE_SIZE; i < count; i++) {
        // trans = trans.conj(entries[i]);
        // TODO add conj function
    }
    return (Vector*)trans;
}

LLVMValueRef VectorCodegen(lisp_object *obj) {
    // TODO
    return NULL;
}

char *VectorToString(lisp_object *obj) {
    // TODO
    return NULL;
}

lisp_object *VectorCopy(lisp_object *obj){
    // TODO
    return NULL;
}


static TransientVector *asTransient(Vector *v) {
    assert(sizeof(TransientVector) == sizeof(Vector));
    TransientVector *ret = malloc(sizeof(*ret));

    memset(ret, '\0', sizeof(*ret));
    memcpy(&ret->obj, &v->obj, sizeof(ret->obj));
    ret->count = v->count;
    ret->shift = v->shift;
    ret->root = editableRoot(v->root);
    for(size_t i = 0; i <NODE_SIZE; i++) {
        lisp_object *o = v->tail[i];
        if(o == NULL)
            break;
        ret->tail[i] = (*(o->copy))(o);
    }

    return ret;
}

static size_t tailoff(TransientVector *v) {
    if(v->count == 0)
        return 0;
    return (v->count - 1) & ~BITMASK;
}

static TransientVector *TransVecConj(TransientVector *v, lisp_object *obj) {
    assert(v->root->editable);
    size_t i = v->count;
    if(i-tailoff(v) < 32) {
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
		newroot->array[0] = v->root;
		// newroot.array[1] = newPath(...)	// TODO
		newshift += 5;
	} else {
		// newroot = pushTail(...)	// TODO
	}
	v->root = newroot;
	v->shift = newshift;
	v->count++;
	return v;
}
