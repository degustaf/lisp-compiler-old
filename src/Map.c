#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Map.h"

#include "ASeq.h"
#include "Cons.h"
#include "Interfaces.h"
#include "intrinsics.h"
#include "nodes.h"
#include "Util.h"

// Support functions

static uint32_t mask(uint32_t hash, size_t shift) {
	return (hash >> shift) & NODE_BITMASK;
}

static uint32_t bitpos(uint32_t hash, size_t shift) {
	return 1 << mask(hash, shift);
}

static size_t index(uint32_t mask, uint32_t bit) {
	return popcount(mask & (bit - 1));
}

// Node interface definition.

typedef struct INode_struct INode;

typedef struct {
	INode* (*assoc)(INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
	INode* (*assoc_thread)(INode *node, bool edit, pthread_t thread_id, size_t shift, uint32_t hash, lisp_object *key, lisp_object *val, bool *addedLeaf);
	INode* (*without)(INode *node, size_t shift, uint32_t hash, lisp_object *key);
	INode* (*without_thread)(INode *node, bool edit, pthread_t thread_id, size_t shift, uint32_t hash, lisp_object *key);
	const MapEntry* (*find)(INode *node, size_t shift, uint32_t hash, lisp_object *key);
	const ISeq* (*nodeSeq)(const INode *node);
	// lisp_object* (*kvreduce)(lisp_object* (*f)(lisp_object*), lisp_object *init);
	// lisp_object* (*fold)(...)
	// Interator (*iterator)(Ifn f)
} INode_vtable;

struct INode_struct {
	lisp_object obj;
	INode_vtable *fns;
};

// BitmapIndexedNode

typedef struct {
	lisp_object obj;
	INode_vtable *fns;
	uint32_t bitmap;
	bool edit;
	pthread_t thread_id;
	size_t count;
	const lisp_object *array[];
} BitmapIndexedNode;

// BitmapIndexedNode function declarations.
BitmapIndexedNode *NewBMINode(bool edit, pthread_t thread_id, uint32_t bitmap, size_t count, const lisp_object **array);
INode* assoc_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
INode* without_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, lisp_object *key);
const MapEntry* find_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, lisp_object *key);
const ISeq* nodeSeq_BitmapIndexed_Node(const INode *node);

INode_vtable BMINode_vtable = {
	assoc_BitmapIndexed_Node,	// assoc
	NULL,						// assoc_thread
	without_BitmapIndexed_Node,	// without
	NULL,						// without_thread
	find_BitmapIndexed_Node,	// find
	nodeSeq_BitmapIndexed_Node	// nodeSeq
};

BitmapIndexedNode _EmptyBMINode = {{BMI_NODE_type, NULL, NULL, NULL, NULL}, &BMINode_vtable, 0, false, (pthread_t)NULL, 0, };
BitmapIndexedNode *EmptyBMINode = &_EmptyBMINode;

// ArrayNode

typedef struct {
	lisp_object obj;
	INode_vtable *fns;
	bool edit;
	pthread_t thread_id;
	size_t count;
	INode *array[NODE_SIZE];
} ArrayNode;

// ArrayNode function declarations.

ArrayNode *NewArrayNode(bool edit, pthread_t thread_id, size_t count, INode **array);
INode* assocArrayNode(INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
static INode* pack(ArrayNode *node, bool edit, pthread_t thread_id, size_t idx);
INode* without(INode *node, size_t shift, uint32_t hash, lisp_object *key);
const MapEntry* findArrayNode(INode *node, size_t shift, uint32_t hash, lisp_object *key);
const ISeq* nodeSeq_ArrayNode(const INode *node);

INode_vtable ArrayNode_vtable = {
	assocArrayNode,		// assoc
	NULL,				// assoc_thread
	without,			// without
	NULL,				// without_thread
	findArrayNode,		// find
	nodeSeq_ArrayNode	// nodeSeq
};

// NodeSeq

typedef struct {
	lisp_object obj;
	const size_t i;
	ISeq *const s;
	const size_t count;
	lisp_object *const array[];
} NodeSeq;

// NodeSeq function declarations.
const NodeSeq *NewNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s);
const NodeSeq *CreateNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s);
const lisp_object* firstNodeSeq(const ISeq*);
const ISeq* nextNodeSeq(const ISeq*);

Seqable_vtable NodeSeq_Seqable_vtable = {
	seqASeq,	//seq
};

ICollection_vtable NodeSeq_ICollection_vtable = {
	countASeq,		// count
	emptyASeq,		// empty
	EquivASeq		// Equiv
};

ISeq_vtable NodeSeq_ISeq_vtable = {
	firstNodeSeq,	// first
	nextNodeSeq,	// next
	moreASeq,		// more
	consASeq,		// cons
};

interfaces NodeSeq_interfaces = {
	&NodeSeq_Seqable_vtable,		// SeqableFns
	NULL,							// ReversibleFns
	&NodeSeq_ICollection_vtable,	// ICollectionFns
	NULL,							// IStackFns
	&NodeSeq_ISeq_vtable,			// ISeqFns
	NULL,							// IVectorFns
	NULL,							// IMapFns
};

// TransientHashMap

typedef struct {
	lisp_object obj;
	bool edit;
	pthread_t thread_id;
	INode *root;
	size_t count;
	bool hasNull;
	const lisp_object *nullValue;
	bool *leafFlag;
} TransientHashMap;

// HashMap

struct HashMap_struct {
	lisp_object obj;
	const size_t count;
	const INode *const root;
	const bool hasNull;
	const lisp_object *const nullValue;
	// const IPersistentMap _meta;
};

// TransientHashMap Function declarations.

static const HashMap *asPersistent(TransientHashMap *thm);
static TransientHashMap *assocTHM(TransientHashMap *thm, const lisp_object *key, const lisp_object *val);

// HashMap Function declarations.

static const HashMap *NewHashMap(int count, INode* root, bool hasNull, const lisp_object *const nullValue);
static TransientHashMap *asTransient(const HashMap *const hm);
const ISeq *seqHashMap(const Seqable *obj);
static size_t countHashMap(const ICollection *ic);

Seqable_vtable HashMap_Seqable_vtable = {
	seqHashMap // seq
};

ICollection_vtable HashMap_ICollection_vtable = {
	countHashMap,	// count
	NULL,	// empty	// TODO
	NULL,	//Equiv		// TODO
};

IMap_vtable HashMap_IMap_vtable = {
	NULL,	// assoc	// TODO
	NULL,	// without	// TODO
	NULL,	// count	// TODO
	NULL,	// entryAt	// TODO
	NULL,	// cons		// TODO
};

interfaces HashMap_interfaces = {
	&HashMap_Seqable_vtable,		// SeqableFns
	NULL,							// ReversibleFns
	&HashMap_ICollection_vtable,	// ICollectionFns
	NULL,							// IStackFns
	NULL,							// ISeqFns
	NULL,							// IVectorFns
	&HashMap_IMap_vtable,			// IMapFns
};

const HashMap _EmptyHashMap = {{HASHMAP_type, NULL, toString, NULL, &HashMap_interfaces}, 0, NULL, false, NULL};
const HashMap *const EmptyHashMap = &_EmptyHashMap;

// BitmapIndexedNode Function Definitions

BitmapIndexedNode *NewBMINode(bool edit, pthread_t thread_id, uint32_t bitmap, size_t count, const lisp_object **array) {
	BitmapIndexedNode *node = malloc(sizeof(*node) + count * sizeof(lisp_object*));
	memcpy(node, EmptyBMINode, sizeof(*node));
	node->edit = edit;
	node->thread_id = thread_id;
	node->bitmap = bitmap;
	node->count = count;
	memcpy(node->array, array, count * sizeof(*array));
	return node;
}

INode* assoc_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	uint32_t bit = bitpos(hash, shift);
	size_t idx = index(BMI_node->bitmap, bit);
	printf("bitmap = %x, bit = %x, idx = %zd\n",BMI_node->bitmap, bit, idx);

	if(BMI_node->bitmap & bit) {
		printf("Inside 'BMI_node->bitmap & bit'\n");
		const lisp_object *lookup_key = BMI_node->array[2*idx];
		const lisp_object *lookup_val = BMI_node->array[2*idx+1];
		if(lookup_key == NULL) {
			INode *n = (INode*)lookup_val;
			n = n->fns->assoc(n, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
			if((lisp_object*)n == lookup_val) {
				return node;
			}
			BMI_node = NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap, BMI_node->count, BMI_node->array);
			BMI_node->array[2*idx+1] = (lisp_object*)n;
			return (INode*)BMI_node;
		}
		if(Equiv(key, lookup_key)) {
			if(val == lookup_val)
				return node;
			BMI_node = NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap, BMI_node->count, BMI_node->array);
			BMI_node->array[2*idx+1] = (lisp_object*)val;
			return (INode*)BMI_node;
		}
		*addedLeaf = true;
		BMI_node = NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap, BMI_node->count, BMI_node->array);
		BMI_node->array[2*idx  ] = key;
		BMI_node->array[2*idx+1] = val;
		return (INode*)BMI_node;
	} else {
		size_t n = popcount(BMI_node->bitmap);
		printf("n = %zd\n", n);
		if(n >= 16) {
			printf("n >= 16\n");
			INode *nodes[NODE_SIZE];
			uint32_t idx = mask(hash, shift);
			nodes[idx] = assoc_BitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
			size_t i, j;
			for(i = j = 0; i < NODE_SIZE; i++) {
				if((BMI_node->bitmap >> i) & 1) {
					if(BMI_node->array[j] == NULL) {
						nodes[i] = (INode*)BMI_node->array[j+1];
					} else {
						nodes[i] = assoc_BitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, HashEq(BMI_node->array[j]),
															BMI_node->array[j], BMI_node->array[j+1], addedLeaf);
					}
					j += 2;
				}
			}
			return (INode*)NewArrayNode(false, (pthread_t)NULL, n + 1, nodes);
		} else {
			printf("n < 16\n");
			const lisp_object *array[2*(n+1)];
			memcpy(&array[0], BMI_node->array, 2 * idx * sizeof(lisp_object*));
			array[2*idx  ] = key;
			array[2*idx+1] = val;
			*addedLeaf = true;
			memcpy(&array[2*(idx+1)], &(BMI_node->array[2*idx]), 2 * (n - idx) * sizeof(lisp_object*));		// Bug from mistranslating from Java
			return (INode*)NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap | bit, BMI_node->count + 2, array);
		}
	}
}

INode* without_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, lisp_object *key) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	uint32_t bit = bitpos(hash, shift);
	if((BMI_node->bitmap & bit) == 0)
		return node;
	size_t idx = index(BMI_node->bitmap, bit);
	const lisp_object *lookup_key = BMI_node->array[2*idx];
	if(lookup_key == NULL) {
		const lisp_object *lookup_val = BMI_node->array[2*idx+1];
		INode *n = (*(((INode*)lookup_val)->fns->without))((INode*)lookup_val, shift + NODE_LOG_SIZE, hash, key);
		if((INode*)lookup_val == n)
			return node;
		if(n) {
			BMI_node = NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap, BMI_node->count, BMI_node->array);
			BMI_node->array[2*idx+1] = (lisp_object*)n;
			return n;
		}
		if(BMI_node->bitmap == bit)
			return NULL;
		const lisp_object *array[BMI_node->count-2];
		memcpy(array, &(BMI_node->array[0]), 2*idx);
		memcpy(&(array[2*idx]), &(BMI_node->array[2*(idx+1)]), BMI_node->count - 2*(idx+1));
		return (INode*)NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap ^ bit, BMI_node->count-2, array);
	}
	if(Equiv(key, lookup_key)) {
		const lisp_object *array[BMI_node->count-2];
		memcpy(array, &(BMI_node->array[0]), 2*idx);
		memcpy(&(array[2*idx]), &(BMI_node->array[2*(idx+1)]), BMI_node->count - 2*(idx+1));
		return (INode*)NewBMINode(false, (pthread_t)NULL, BMI_node->bitmap ^ bit, BMI_node->count-2, array);
	}
	return node;
}

const MapEntry* find_BitmapIndexed_Node(INode *node, size_t shift, uint32_t hash, lisp_object *key) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	uint32_t bit = bitpos(hash, shift);
	if((BMI_node->bitmap & bit) == 0)
		return NULL;
	size_t idx = index(BMI_node->bitmap, bit);
	const lisp_object *lookup_key = BMI_node->array[2*idx];
	const lisp_object *lookup_val = BMI_node->array[2*idx+1];
	if(lookup_key == NULL)
		return ((INode*)lookup_val)->fns->find((INode*)lookup_val, shift + NODE_LOG_SIZE, hash, key);
	if(Equiv(key, lookup_key)) {
		MapEntry *ret = malloc(sizeof(*ret));
		ret->key = lookup_key;
		ret->val = lookup_val;
		return ret;
	}
	return NULL;
}

const ISeq* nodeSeq_BitmapIndexed_Node(const INode *node) {
	printf("In nodeSeq_BitmapIndexed_Node\n");
	fflush(stdout);
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	return (const ISeq*) CreateNodeSeq(BMI_node->array, BMI_node->count, 0, NULL);
}

// ArrayNode function Definitions.

ArrayNode *NewArrayNode(bool edit, pthread_t thread_id, size_t count, INode **array) {
	ArrayNode *node = malloc(sizeof(*node));
	memset(node, '\0', sizeof(*node));
	node->obj.type = ARRAY_NODE_type;
	node->fns = &ArrayNode_vtable;
	node->edit = edit;
	node->thread_id = thread_id;
	node->count = count;
	memcpy(node->array, array, NODE_SIZE * sizeof(*array));
	return node;
}

INode* assocArrayNode(INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	INode *n = anode->array[idx];
	if(n == NULL) {
		INode *array[NODE_SIZE];
		memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
		array[idx] = assoc_BitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
		return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count+1, array);
	}
	INode *n2 = n->fns->assoc(n, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
	if(n == n2)
		return node;
	INode *array[NODE_SIZE];
	memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
	array[idx] = n2;
	return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count, array);
}

static INode* pack(ArrayNode *node, bool edit, pthread_t thread_id, size_t idx) {
	const lisp_object *array[2*(node->count - 1)];
	memset(&array[0], '\0', sizeof(array));
	size_t j = 1;
	uint32_t bitmap = 0;
	for(size_t i=0; i<idx; i++) {
		if(node->array[i] != NULL) {
			array[j] = (lisp_object*)node->array[i];
			bitmap |= (1 << i);
			j += 2;
		}
	}
	for(size_t i=idx+1; i<NODE_SIZE; i++) {
		if(node->array[i] != NULL) {
			array[j] = (lisp_object*)node->array[i];
			bitmap |= (1 << i);
			j += 2;
		}
	}
	return (INode*)NewBMINode(edit, thread_id, bitmap, 2*(node->count-1), array);
}

INode* without(INode *node, size_t shift, uint32_t hash, lisp_object *key) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	INode *n = anode->array[idx];
	if(n == NULL)
		return node;
	INode *n2 = n->fns->without(n, shift + NODE_LOG_SIZE, hash, key);
	if(n2 == n)
		return node;
	if(n2 == NULL) {
		if(anode->count <= 8)
			return pack(anode, anode->edit, anode->thread_id, idx);
		INode *array[NODE_SIZE];
		memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
		array[idx] = n2;
		return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count-1, array);
	}
	INode *array[NODE_SIZE];
	memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
	array[idx] = n2;
	return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count, array);
}

const MapEntry* findArrayNode(INode *node, size_t shift, uint32_t hash, lisp_object *key) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	INode *n = anode->array[idx];
	if(n == NULL)
		return NULL;
	return n->fns->find(n, shift + NODE_LOG_SIZE, hash, key);
}

const ISeq* nodeSeq_ArrayNode(const INode *node) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	return (const ISeq*) CreateNodeSeq((const lisp_object**) anode->array, NODE_SIZE, 0, NULL);

	return NULL;
}

// NodeSeq Function Definitions.

const NodeSeq *NewNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s) {
	assert(i <= count);
	assert((s == NULL) || isISeq(&s->obj));
	NodeSeq *ret = malloc(sizeof(*ret) + count * sizeof(lisp_object*));
	ret->obj.type = NODESEQ_type;
	ret->obj.codegen = NULL;
	ret->obj.toString = toString;
	ret->obj.fns = &NodeSeq_interfaces;

	memcpy((void*)&(ret->i), &i, sizeof(i));
	memcpy((void*)&(ret->s), &s, sizeof(s));
	memcpy((void*)&(ret->count), &count, sizeof(count));
	memcpy((void*) ret->array, array, count * sizeof(*array));
	return ret;
}

const NodeSeq *CreateNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s) {
	printf("In CreateNodeSeq.\n");
	printf("count = %zd, i = %zd\n", count, i);
	fflush(stdout);
	// assert(i <= count);
	if(s) return NewNodeSeq(array, count, i, s);
	printf("s is NULL\n");
	fflush(stdout);

	for(size_t j = i; j < count; j+=2) {
		printf("In for loop with i = %zd, and j = %zd\n", i, j);
		fflush(stdout);
		if(array[j]) return NewNodeSeq(array, count, j, NULL);
		printf("array[j] is NULL.\n");
		fflush(stdout);

		INode *node = (INode*) array[j+1];
		printf("Got node.\n");
		fflush(stdout);
		printf("node = %p\n", (void*)node);
		fflush(stdout);
		if(node) {
			printf("node is not NULL\n");
			printf("node is a %s\n", object_type_string[node->obj.type]);
			fflush(stdout);
			printf("node = %p\n", (void*)node);
			fflush(stdout);
			printf("node = %p\n", (void*)node->fns);
			fflush(stdout);
			printf("node = %p\n", (void*)(size_t)node->fns->nodeSeq);
			fflush(stdout);
			const ISeq *NodeSeq = node->fns->nodeSeq(node);
			printf("Got NodeSeq.\n");
			fflush(stdout);
			if(NodeSeq) return NewNodeSeq(array, count, j+2, NodeSeq);
		}
	}

	return NULL;
}

const lisp_object* firstNodeSeq(const ISeq *o) {
	assert(o->obj.type == NODESEQ_type);
	NodeSeq *ns = (NodeSeq*) o;

	if(ns->s)
		return ns->s->obj.fns->ISeqFns->first(ns->s);

	MapEntry *ret = malloc(sizeof(*ret));
	ret->key = ns->array[ns->i];
	ret->val = ns->array[ns->i+1];
	return (lisp_object*) ret;
}

const ISeq* nextNodeSeq(const ISeq *o) {
	printf("In nextNodeSeq.\n");
	fflush(stdout);
	assert(o->obj.type == NODESEQ_type);
	NodeSeq *ns = (NodeSeq*) o;

	if(ns->s)
		return (ISeq*) CreateNodeSeq((const lisp_object**) ns->array, ns->count, ns->i, ns->s->obj.fns->ISeqFns->next(ns->s));
	printf("ns->s is NULL.\n");
	fflush(stdout);
	return (ISeq*) CreateNodeSeq((const lisp_object**) ns->array, ns->count, ns->i+2, NULL);
}

// TransientHashMap Function Definitions.

static const HashMap *asPersistent(TransientHashMap *thm) {
	assert(thm->edit);
	thm->edit = false;
	return NewHashMap(thm->count, thm->root, thm->hasNull, thm->nullValue);
}

static TransientHashMap *assocTHM(TransientHashMap *thm, const lisp_object *key, const lisp_object *val) {
	assert(thm->edit);
	if(key == NULL) {
		thm->nullValue = val;
		if(!thm->hasNull) {
			thm->hasNull = true;
			thm->count++;
		}
		return thm;
	}

	bool leafFlag = false;
	if(thm->root == NULL) thm->root = (INode*)EmptyBMINode;
	INode *n = thm->root->fns->assoc(thm->root, 0, HashEq(key), key, val, &leafFlag);
	if(n != thm->root) thm->root = n;
	if(leafFlag) thm->count++;
	return thm;
}

// HashMap Function Definitions.

const HashMap *CreateHashMap(size_t count, const lisp_object **entries) {
	printf("In CreateHashMap.\n");
	fflush(stdout);
	TransientHashMap *ret = asTransient(EmptyHashMap);
	printf("count = %zd\n", ret->count);
	for(size_t i=0; i<count; i+=2) {
		printf("%p => %p\n", (void*) entries[i], (void*) entries[i+1]);
		ret = assocTHM(ret, entries[i], entries[i+1]);
		printf("count = %zd\n", ret->count);
	}
	const HashMap *result = asPersistent(ret);
	printf("count = %zd\n", result->count);
	return result;
}

static const HashMap *NewHashMap(int count, INode* root, bool hasNull, const lisp_object *const nullValue) {
	HashMap *ret = malloc(sizeof(*ret));
	ret->obj.type = HASHMAP_type;
	ret->obj.codegen = NULL;
	ret->obj.toString = toString;
	ret->obj.fns = &HashMap_interfaces;
	memcpy((void*) &(ret->count), &count, sizeof(count));
	memcpy((void*) &(ret->root), &root, sizeof(root));
	memcpy((void*) &(ret->hasNull), &hasNull, sizeof(hasNull));
	memcpy((void*) &(ret->nullValue), &nullValue, sizeof(nullValue));

	return ret;
}

static TransientHashMap *asTransient(const HashMap *const hm) {
	printf("In asTransient.\n");
	printf("thm has size %zd\n", sizeof(TransientHashMap));
	fflush(stdout);
	TransientHashMap *thm = malloc(sizeof(*thm));
	printf("Allocated thm.\n");
	fflush(stdout);
	thm->obj.type = TRANSIENTHASHMAP_type;
	thm->obj.codegen = NULL;
	thm->obj.toString = NULL;
	printf("populated thm->obj.\n");
	fflush(stdout);
	thm->edit = true;
	thm->thread_id = pthread_self();
	thm->root = (INode*)(hm->root);
	thm->count = hm->count;
	thm->hasNull = hm->hasNull;
	thm->nullValue = hm->nullValue;

	printf("Leaving asTransient.");
	fflush(stdout);
	return thm;
}

const ISeq *seqHashMap(const Seqable *obj) {
	printf("In seqHashMap\n");
	fflush(stdout);
	assert(obj->obj.type == HASHMAP_type);
	HashMap *hm = (HashMap*) obj;

	const ISeq *s = (hm->root ? hm->root->fns->nodeSeq(hm->root) : NULL);
	printf("Got s\n");
	fflush(stdout);

	return hm->hasNull ? (const ISeq*) NewCons(hm->nullValue, s) : s;
}

static size_t countHashMap(const ICollection *ic) {
	assert(ic->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) ic;
	return hm->count;
}
