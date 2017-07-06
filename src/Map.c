#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "Map.h"

#include "AFn.h"
#include "ASeq.h"
#include "Cons.h"
#include "Error.h"
#include "gc.h"
#include "Interfaces.h"
#include "intrinsics.h"
#include "lisp_pthread.h"
#include "MapEntry.h"
#include "nodes.h"
#include "Numbers.h"
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

typedef struct {	// INode_vtable
	const INode* (*assoc)(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
	INode* (*assoc_thread)(INode *node, bool edit, pthread_t thread_id, size_t shift, uint32_t hash, lisp_object *key, lisp_object *val, bool *addedLeaf);
	const INode* (*without)(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
	INode* (*without_thread)(INode *node, bool edit, pthread_t thread_id, size_t shift, uint32_t hash, lisp_object *key);
	const MapEntry* (*find)(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
	const ISeq* (*nodeSeq)(const INode *node);
	// lisp_object* (*kvreduce)(lisp_object* (*f)(lisp_object*), lisp_object *init);
	// lisp_object* (*fold)(...)
	// Interator (*iterator)(Ifn f)
} INode_vtable;

struct INode_struct {
	lisp_object obj;
	const INode_vtable *fns;
};

// INode function declarations.

static const INode* createNode(bool edit, pthread_t thread_id, size_t shift, const lisp_object *key1, const lisp_object *val1, uint32_t key2hash, const lisp_object *key2, const lisp_object *val2);

// BitmapIndexedNode

typedef struct {
	lisp_object obj;
	const INode_vtable *fns;
	uint32_t bitmap;
	bool edit;
	pthread_t thread_id;
	size_t count;
	const lisp_object *array[];
} BitmapIndexedNode;

// BitmapIndexedNode function declarations.
BitmapIndexedNode *NewBMINode(bool edit, pthread_t thread_id, uint32_t bitmap, size_t count, const lisp_object **array);
const INode* assocBitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
const INode* without_BitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
const MapEntry* find_BitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
const ISeq* nodeSeq_BitmapIndexed_Node(const INode *node);

const INode_vtable BMINode_vtable = {
	assocBitmapIndexed_Node,	// assoc
	NULL,						// assoc_thread	// TODO
	without_BitmapIndexed_Node,	// without
	NULL,						// without_thread	// TODO
	find_BitmapIndexed_Node,	// find
	nodeSeq_BitmapIndexed_Node	// nodeSeq
};

BitmapIndexedNode _EmptyBMINode = {{BMI_NODE_type, sizeof(BitmapIndexedNode), NULL, NULL, NULL, NULL}, &BMINode_vtable, 0, false, (pthread_t)NULL, 0, };
BitmapIndexedNode *EmptyBMINode = &_EmptyBMINode;

// ArrayNode

typedef struct {
	lisp_object obj;
	const INode_vtable *fns;
	bool edit;
	pthread_t thread_id;
	size_t count;
	const INode *array[NODE_SIZE];
} ArrayNode;

// ArrayNode function declarations.

ArrayNode *NewArrayNode(bool edit, pthread_t thread_id, size_t count, const INode **array);
const INode* assocArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
static INode* pack(ArrayNode *node, bool edit, pthread_t thread_id, size_t idx);
const INode* withoutArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
const MapEntry* findArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
const ISeq* nodeSeq_ArrayNode(const INode *node);

const INode_vtable ArrayNode_vtable = {
	assocArrayNode,		// assoc
	NULL,				// assoc_thread	// TODO
	withoutArrayNode,	// without
	NULL,				// without_thread	// TODO
	findArrayNode,		// find
	nodeSeq_ArrayNode	// nodeSeq
};

// CollisionNode

typedef struct {	// CollisionNode
	lisp_object obj;
	const INode_vtable *fns;
	uint32_t hash;
	size_t count;
	bool edit;
	pthread_t thread_id;
	const lisp_object *array[];
} CollisionNode;

// CollisionNode function declarations.

static CollisionNode *NewCollisionNode(bool edit, pthread_t thread_id, uint32_t hash, size_t count, const lisp_object **array);
static const INode* assocCollisionNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf);
static const INode* withoutCollisionNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
static const MapEntry* findCollisionNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key);
static const ISeq* nodeSeqCollisionNode(const INode *node);
static int findIndex(const CollisionNode *cnode, const lisp_object *key);

const INode_vtable CollisionNode_vtable = {
	assocCollisionNode,		// assoc
	NULL,					// assoc_thread	// TODO
	withoutCollisionNode,	// without
	NULL,					// without_thread	// TODO
	findCollisionNode,		// find
	nodeSeqCollisionNode	// nodeSeq
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

const Seqable_vtable NodeSeq_Seqable_vtable = {
	seqASeq,	//seq
};

const ICollection_vtable NodeSeq_ICollection_vtable = {
	countASeq,					// count
	(ICollectionFn1)consASeq,	// cons
	emptyASeq,					// empty
	EquivASeq					// Equiv
};

const ISeq_vtable NodeSeq_ISeq_vtable = {
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
	NULL,							// IFnFns
	NULL,							// IVectorFns
	NULL,							// IMapFns
};

// ArrayNodeSeq

typedef struct {
	lisp_object obj;
	const size_t i;
	const ISeq *const s;
	const INode *array[NODE_SIZE];
} ArrayNodeSeq;

// NodeSeq function declarations.
const ArrayNodeSeq *NewArrayNodeSeq(const INode **array, size_t i, const ISeq *s);
const ArrayNodeSeq *CreateArrayNodeSeq(const INode **array, size_t i, const ISeq *s);
const lisp_object* firstArrayNodeSeq(const ISeq*);
const ISeq* nextArrayNodeSeq(const ISeq*);

const ISeq_vtable ArrayNodeSeq_ISeq_vtable = {
	firstArrayNodeSeq,	// first
	nextArrayNodeSeq,	// next
	moreASeq,			// more
	consASeq,			// cons
};

interfaces ArrayNodeSeq_interfaces = {
	&NodeSeq_Seqable_vtable,		// SeqableFns
	NULL,							// ReversibleFns
	&NodeSeq_ICollection_vtable,	// ICollectionFns
	NULL,							// IStackFns
	&ArrayNodeSeq_ISeq_vtable,		// ISeqFns
	NULL,							// IFnFns
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

static const HashMap *NewHashMap(int count, const INode* root, bool hasNull, const lisp_object *const nullValue);
static TransientHashMap *asTransient(const HashMap *const hm);
const ISeq *seqHashMap(const Seqable *obj);
static size_t countHashMap(const ICollection *ic);
static const ICollection* emptyHashMap(void);
static bool EquivHashMap(const ICollection*, const lisp_object*);
static const lisp_object* invoke1HashMap(const IFn*, const lisp_object*);
static const lisp_object* invoke2HashMap(const IFn*, const lisp_object*, const lisp_object*);
static bool containsKeyHashMap(const IMap*, const lisp_object*);
static const IMap* assocHashMap(const IMap*, const lisp_object*, const lisp_object*);
static const IMap* withoutHashMap(const IMap*, const lisp_object*);
static const MapEntry* entryAtHashMap(const IMap*, const lisp_object*);
static const IMap* consHashMap(const IMap*, const lisp_object*);
static bool EqualsHashMap(const lisp_object *x, const lisp_object *y);

const Seqable_vtable HashMap_Seqable_vtable = {
	seqHashMap // seq
};

const ICollection_vtable HashMap_ICollection_vtable = {
	countHashMap,					// count
	(ICollectionFn1)consHashMap,	// cons
	emptyHashMap,					// empty
	EquivHashMap,					// Equiv	
};

const IFn_vtable HashMap_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1HashMap,	// invoke1
	invoke2HashMap,	// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
};

const IMap_vtable HashMap_IMap_vtable = {
	containsKeyHashMap,	// containsKey
	assocHashMap,		// assoc
	withoutHashMap,		// without
	entryAtHashMap,		// entryAt
	consHashMap,		// cons	
};

interfaces HashMap_interfaces = {
	&HashMap_Seqable_vtable,		// SeqableFns
	NULL,							// ReversibleFns
	&HashMap_ICollection_vtable,	// ICollectionFns
	NULL,							// IStackFns
	NULL,							// ISeqFns
	&HashMap_IFn_vtable,			// IFnFns
	NULL,							// IVectorFns
	&HashMap_IMap_vtable,			// IMapFns
};

const HashMap _EmptyHashMap = {{HASHMAP_type, sizeof(HashMap), toString, EqualsHashMap, (IMap*)&_EmptyHashMap, &HashMap_interfaces}, 0, NULL, false, NULL};
const HashMap *const EmptyHashMap = &_EmptyHashMap;

// INode Function Definitions

static const INode* createNode(__attribute__((unused)) bool edit, __attribute__((unused)) pthread_t thread_id, size_t shift, const lisp_object *key1, const lisp_object *val1, uint32_t key2hash, const lisp_object *key2, const lisp_object *val2) {
	// TODO make this respect threads, which requires assoc_thread.
	uint32_t key1hash = HashEq(key1);
	if(key1hash == key2hash) {
		const lisp_object *array[4] = {key1, val1, key2, val2};
		return (INode*) NewCollisionNode(false, NULL, key1hash, 2, array);
	}
	const INode *ret = (INode*) EmptyBMINode;
	bool addedLeaf = false;
	ret = ret->fns->assoc(ret, shift, key1hash, key1, val1, &addedLeaf);
	ret = ret->fns->assoc(ret, shift, key2hash, key2, val2, &addedLeaf);
	return ret;
}

// BitmapIndexedNode Function Definitions

BitmapIndexedNode *NewBMINode(bool edit, pthread_t thread_id, uint32_t bitmap, size_t count, const lisp_object **array) {
	BitmapIndexedNode *node = GC_MALLOC(sizeof(*node) + count * sizeof(lisp_object*));
	memcpy(node, EmptyBMINode, sizeof(*node));
	node->edit = edit;
	node->thread_id = thread_id;
	node->bitmap = bitmap;
	node->count = count;
	memcpy(node->array, array, count * sizeof(*array));
	return node;
}

const INode* assocBitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	uint32_t bit = bitpos(hash, shift);
	size_t idx = index(BMI_node->bitmap, bit);

	if(BMI_node->bitmap & bit) {
		const lisp_object *lookup_key = BMI_node->array[2*idx];
		const lisp_object *lookup_val = BMI_node->array[2*idx+1];
		if(lookup_key == NULL) {
			const INode *n = (INode*)lookup_val;
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
		BMI_node->array[2*idx  ] = NULL;
		BMI_node->array[2*idx+1] = (lisp_object*) createNode(true, pthread_self(), shift + NODE_LOG_SIZE, lookup_key, lookup_val, hash, key, val);
		return (INode*)BMI_node;
	} else {
		size_t n = popcount(BMI_node->bitmap);
		if(n >= 16) {
			const INode *nodes[NODE_SIZE];
			memset(nodes, 0, sizeof(nodes));
			uint32_t idx = mask(hash, shift);
			nodes[idx] = assocBitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
			size_t i, j;
			for(i = j = 0; i < NODE_SIZE; i++) {
				if((BMI_node->bitmap >> i) & 1) {
					if(BMI_node->array[j] == NULL) {
						nodes[i] = (INode*)BMI_node->array[j+1];
					} else {
						nodes[i] = assocBitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, HashEq(BMI_node->array[j]),
															BMI_node->array[j], BMI_node->array[j+1], addedLeaf);
					}
					j += 2;
				}
			}
			return (const INode*)NewArrayNode(false, (pthread_t)NULL, n + 1, nodes);
		}
		else {
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

const INode* without_BitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	uint32_t bit = bitpos(hash, shift);
	if((BMI_node->bitmap & bit) == 0)
		return node;
	size_t idx = index(BMI_node->bitmap, bit);
	const lisp_object *lookup_key = BMI_node->array[2*idx];
	if(lookup_key == NULL) {
		const lisp_object *lookup_val = BMI_node->array[2*idx+1];
		const INode *n = ((INode*)lookup_val)->fns->without((INode*)lookup_val, shift + NODE_LOG_SIZE, hash, key);
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
		memcpy(array,			&(BMI_node->array[0]),			2*idx);
		memcpy(&(array[2*idx]),	&(BMI_node->array[2*(idx+1)]),	BMI_node->count - 2*(idx+1));
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

const MapEntry* find_BitmapIndexed_Node(const INode *node, size_t shift, uint32_t hash, const lisp_object *key) {
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
		MapEntry *ret = GC_MALLOC(sizeof(*ret));
		ret->key = lookup_key;
		ret->val = lookup_val;
		return ret;
	}
	return NULL;
}

const ISeq* nodeSeq_BitmapIndexed_Node(const INode *node) {
	assert(node->obj.type == BMI_NODE_type);
	BitmapIndexedNode *BMI_node = (BitmapIndexedNode*)node;
	return (const ISeq*) CreateNodeSeq(BMI_node->array, BMI_node->count, 0, NULL);
}

// ArrayNode function Definitions.

ArrayNode *NewArrayNode(bool edit, pthread_t thread_id, size_t count, const INode **array) {
	ArrayNode *node = GC_MALLOC(sizeof(*node));
	node->obj.type = ARRAY_NODE_type;
	node->obj.size = sizeof(ArrayNode);
	node->fns = &ArrayNode_vtable;
	node->edit = edit;
	node->thread_id = thread_id;
	node->count = count;
	memcpy(node->array, array, NODE_SIZE * sizeof(*array));
	return node;
}

const INode* assocArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	const INode *n = anode->array[idx];
	if(n == NULL) {
		const INode *array[NODE_SIZE];
		memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
		array[idx] = assocBitmapIndexed_Node((INode*)EmptyBMINode, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
		return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count+1, array);
	}
	const INode *n2 = n->fns->assoc(n, shift + NODE_LOG_SIZE, hash, key, val, addedLeaf);
	if(n == n2)
		return node;
	const INode *array[NODE_SIZE];
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

const INode* withoutArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	const INode *n = anode->array[idx];
	if(n == NULL)
		return node;
	const INode *n2 = n->fns->without(n, shift + NODE_LOG_SIZE, hash, key);
	if(n2 == n)
		return node;
	if(n2 == NULL) {
		if(anode->count <= 8)
			return pack(anode, anode->edit, anode->thread_id, idx);
		const INode *array[NODE_SIZE];
		memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
		array[idx] = n2;
		return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count-1, array);
	}
	const INode *array[NODE_SIZE];
	memcpy(&array[0], anode->array, NODE_SIZE * sizeof(INode*));
	array[idx] = n2;
	return (INode*)NewArrayNode(false, (pthread_t)NULL, anode->count, array);
}

const MapEntry* findArrayNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;
	uint32_t idx = mask(hash, shift);
	assert(idx < NODE_SIZE);
	const INode *n = anode->array[idx];
	if(n == NULL)
		return NULL;
	return n->fns->find(n, shift + NODE_LOG_SIZE, hash, key);
}

const ISeq* nodeSeq_ArrayNode(const INode *node) {
	assert(node->obj.type == ARRAY_NODE_type);
	ArrayNode *anode = (ArrayNode*)node;

	return (const ISeq*) CreateArrayNodeSeq(&anode->array[0], 0, NULL);
}

// CollisionNode Function Definitions

static CollisionNode *NewCollisionNode(bool edit, pthread_t thread_id, uint32_t hash, size_t count, const lisp_object **array) {
	CollisionNode *node = GC_MALLOC(sizeof(*node) + 2 * count * sizeof(lisp_object*));
	node->obj.type = COLLISIONNODE_type;
	node->obj.size = sizeof(CollisionNode);
	node->fns = &CollisionNode_vtable;
	node->edit = edit;
	node->thread_id = thread_id;
	node->hash = hash;
	node->count = count;
	memcpy(node->array, array, 2 * count * sizeof(*array));
	return node;
}

static const INode* assocCollisionNode(const INode *node, size_t shift, uint32_t hash, const lisp_object *key, const lisp_object *val, bool *addedLeaf) {
	assert(node->obj.type == COLLISIONNODE_type);
	CollisionNode *cnode = (CollisionNode*) node;

	if(hash == cnode->hash) {
		int idx = findIndex(cnode, key);
		if(idx != -1) {
			if(cnode->array[idx+1] == val)
				return node;
			CollisionNode *c = NewCollisionNode(false, NULL, hash, cnode->count, cnode->array);
			c->array[idx+1] = val;
		}
		const lisp_object *array[2*(cnode->count + 1)];
		memcpy(&array[0], cnode->array, 2 * cnode->count * sizeof(lisp_object*));
		array[2*cnode->count  ] = key;
		array[2*cnode->count+1] = val;
		*addedLeaf = true;
		return (INode*) NewCollisionNode(cnode->edit, cnode->thread_id, hash, cnode->count+1, array);
	}
	const lisp_object *array[2] = {NULL, (const lisp_object*)cnode};
	INode *ret = (INode*) NewBMINode(false, NULL, hash, 2, array);
	return ret->fns->assoc(ret, shift, hash, key, val, addedLeaf);
}

static const INode* withoutCollisionNode(const INode *node, __attribute__((unused)) size_t shift, uint32_t hash, const lisp_object *key) {
	assert(node->obj.type == COLLISIONNODE_type);
	CollisionNode *cnode = (CollisionNode*) node;

	int i = findIndex(cnode, key);
	if(i == -1)
		return node;
	if(cnode->count == 1)
		return NULL;
	const lisp_object *array[2*(cnode->count - 1)];
	memcpy(array,		&(cnode->array[0]),		i);
	memcpy(&(array[i]),	&(cnode->array[i+2]),	2*(cnode->count-1) - i);
	return (INode*) NewCollisionNode(false, NULL, hash, cnode->count - 1, array);
}

static const MapEntry* findCollisionNode(const INode *node, __attribute__((unused)) size_t shift, __attribute__((unused)) uint32_t hash, const lisp_object *key) {
	assert(node->obj.type == COLLISIONNODE_type);
	CollisionNode *cnode = (CollisionNode*) node;

	int i = findIndex(cnode, key);
	if(i == -1)
		return NULL;
	if(Equiv(key, cnode->array[i])) {
		MapEntry *ret = GC_MALLOC(sizeof(*ret));
		ret->key = cnode->array[i];
		ret->val = cnode->array[i+1];
		return ret;
	}
	return NULL;
}

static const ISeq* nodeSeqCollisionNode(const INode *node) {
	assert(node->obj.type == COLLISIONNODE_type);
	CollisionNode *cnode = (CollisionNode*) node;

	return (ISeq*) CreateNodeSeq(cnode->array, cnode->count, 0, NULL);
}

static int findIndex(const CollisionNode *cnode, const lisp_object *key) {
	for(int i = 0; i<2*(int)cnode->count; i+=2) {
		if(Equiv(key, cnode->array[i]))
			return i;
	}
	return -1;
}

// NodeSeq Function Definitions.

const NodeSeq *NewNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s) {
	assert(i <= count);
	assert((s == NULL) || isISeq(&s->obj));
	NodeSeq *ret = GC_MALLOC(sizeof(*ret) + count * sizeof(lisp_object*));
	ret->obj.type = NODESEQ_type;
	ret->obj.size = sizeof(NodeSeq);
	ret->obj.toString = toString;
	ret->obj.fns = &NodeSeq_interfaces;

	memcpy((void*)&(ret->i), &i, sizeof(i));
	memcpy((void*)&(ret->s), &s, sizeof(s));
	memcpy((void*)&(ret->count), &count, sizeof(count));
	memcpy((void*) ret->array, array, count * sizeof(*array));
	return ret;
}

const NodeSeq *CreateNodeSeq(const lisp_object **array, size_t count, size_t i, const ISeq *s) {
	// assert(i <= count);
	if(s) return NewNodeSeq(array, count, i, s);

	for(size_t j = i; j < count; j+=2) {
		if(array[j]) return NewNodeSeq(array, count, j, NULL);

		INode *node = (INode*) array[j+1];
		if(node) {
			const ISeq *NodeSeq = node->fns->nodeSeq(node);
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

	MapEntry *ret = GC_MALLOC(sizeof(*ret));
	ret->key = ns->array[ns->i];
	ret->val = ns->array[ns->i+1];
	return (lisp_object*) ret;
}

const ISeq* nextNodeSeq(const ISeq *o) {
	assert(o->obj.type == NODESEQ_type);
	NodeSeq *ns = (NodeSeq*) o;

	if(ns->s)
		return (ISeq*) CreateNodeSeq((const lisp_object**) ns->array, ns->count, ns->i, ns->s->obj.fns->ISeqFns->next(ns->s));
	return (ISeq*) CreateNodeSeq((const lisp_object**) ns->array, ns->count, ns->i+2, NULL);
}

// ArrayNodeSeq Function Definitions.

const ArrayNodeSeq *NewArrayNodeSeq(const INode **array, size_t i, const ISeq *s) {
	assert(i <= NODE_SIZE);
	assert((s == NULL) || isISeq(&s->obj));
	ArrayNodeSeq *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = ARRAYNODESEQ_type;
	ret->obj.size = sizeof(ArrayNodeSeq);
	ret->obj.toString = toString;
	ret->obj.fns = &ArrayNodeSeq_interfaces;

	memcpy((void*)&(ret->i), &i, sizeof(i));
	memcpy((void*)&(ret->s), &s, sizeof(s));
	memcpy((void*) ret->array, array, NODE_SIZE * sizeof(*array));
	return ret;
}

const ArrayNodeSeq *CreateArrayNodeSeq(const INode **array, size_t i, const ISeq *s) {
	if(s) return NewArrayNodeSeq(array, i, s);

	for(size_t j = i; j < NODE_SIZE; j++) {
		const INode *node = array[j];
		if(node) {
			const ISeq *NodeSeq = node->fns->nodeSeq(node);
			if(NodeSeq) return NewArrayNodeSeq(array, j+1, NodeSeq);
		}
	}

	return NULL;
}

const lisp_object* firstArrayNodeSeq(const ISeq *o) {
	assert(o->obj.type == ARRAYNODESEQ_type);
	ArrayNodeSeq *ns = (ArrayNodeSeq*) o;

	return ns->s->obj.fns->ISeqFns->first(ns->s);
}

const ISeq* nextArrayNodeSeq(const ISeq *o) {
	assert(o->obj.type == ARRAYNODESEQ_type);
	ArrayNodeSeq *ns = (ArrayNodeSeq*) o;

	return (ISeq*) CreateArrayNodeSeq(ns->array, ns->i, ns->s->obj.fns->ISeqFns->next(ns->s));
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
	const INode *n = thm->root->fns->assoc(thm->root, 0, HashEq(key), key, val, &leafFlag);
	if(n != thm->root) thm->root = (INode*)n;
	if(leafFlag) thm->count++;
	return thm;
}

// HashMap Function Definitions.

const HashMap *CreateHashMap(size_t count, const lisp_object **entries) {
	TransientHashMap *ret = asTransient(EmptyHashMap);
	for(size_t i=0; i<count; i+=2) {
		ret = assocTHM(ret, entries[i], entries[i+1]);
	}
	const HashMap *result = asPersistent(ret);
	return result;
}

static const HashMap *NewHashMap(int count, const INode* root, bool hasNull, const lisp_object *const nullValue) {
	HashMap *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = HASHMAP_type;
	ret->obj.size = sizeof(HashMap);
	ret->obj.toString = toString;
	ret->obj.fns = &HashMap_interfaces;
	memcpy((void*) &(ret->count), &count, sizeof(count));
	memcpy((void*) &(ret->root), &root, sizeof(root));
	memcpy((void*) &(ret->hasNull), &hasNull, sizeof(hasNull));
	memcpy((void*) &(ret->nullValue), &nullValue, sizeof(nullValue));

	return ret;
}

static TransientHashMap *asTransient(const HashMap *const hm) {
	TransientHashMap *thm = GC_MALLOC(sizeof(*thm));
	thm->obj.type = TRANSIENTHASHMAP_type;
	thm->obj.toString = NULL;
	thm->edit = true;
	thm->thread_id = pthread_self();
	thm->root = (INode*)(hm->root);
	thm->count = hm->count;
	thm->hasNull = hm->hasNull;
	thm->nullValue = hm->nullValue;

	return thm;
}

const ISeq *seqHashMap(const Seqable *obj) {
	assert(obj->obj.type == HASHMAP_type);
	HashMap *hm = (HashMap*) obj;

	const ISeq *s = (hm->root ? hm->root->fns->nodeSeq(hm->root) : NULL);

	return hm->hasNull ? (const ISeq*) NewCons(hm->nullValue, s) : s;
}

static const ICollection* emptyHashMap(void) {
	return (ICollection*) EmptyHashMap;
}

static bool EquivHashMap(const ICollection *ic, const lisp_object *obj) {
	assert(ic->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) ic;

	if(!isIMap(obj)) return false;

	const IMap *im = (IMap*) obj;
	if(im->obj.fns->ICollectionFns->count((const ICollection*)im) != hm->count) return false;

	for(const ISeq *s = seqHashMap((const Seqable*)hm); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *me = (const MapEntry*) s->obj.fns->ISeqFns->first(s);
		const MapEntry *me2 = im->obj.fns->IMapFns->entryAt(im, me->key);
		if(me2 == NULL || !Equiv(me2->val, me->val)) return false;
	}

	return true;

}

static const lisp_object* invoke1HashMap(const IFn *f, const lisp_object *key) {
	assert(f->obj.type == HASHMAP_type);
	return invoke2HashMap(f, key, NULL);
}

static const lisp_object* invoke2HashMap(const IFn *f, const lisp_object *key, const lisp_object *NotFound) {
	assert(f->obj.type == HASHMAP_type);
	const HashMap *hm = (HashMap*) f;

	if(key == NULL)
		return hm->hasNull ? hm->nullValue : NotFound;
	if(hm->root) {
		const MapEntry *me = hm->root->fns->find(hm->root, 0, HashEq(key), key);
		return me ? me->val : NotFound;
	}
	return NotFound;
}

static size_t countHashMap(const ICollection *ic) {
	assert(ic->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) ic;
	return hm->count;
}

static bool containsKeyHashMap(const IMap *im, const lisp_object *key) {
	assert(im->obj.type == HASHMAP_type);
	const HashMap *hm = (HashMap*)im;

	if(key == NULL)
		return hm->hasNull;
	return hm->root ? hm->root->fns->find(hm->root, 0, HashEq(key), key) == NULL : false;
}

static const IMap* assocHashMap(const IMap *im, const lisp_object *key, const lisp_object *val) {
	assert(im->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) im;

	if(key == NULL) {
		if(hm->hasNull && hm->nullValue == val)
			return im;
		return (IMap*) NewHashMap(hm->hasNull ? hm->count : hm->count+1, hm->root, true, val);
	}

	bool addedLeaf = false;
	const INode *newRoot = hm->root ? hm->root : (INode*) EmptyBMINode;
	newRoot = newRoot->fns->assoc(newRoot, 0, HashEq(key), key, val, &addedLeaf);
	if(newRoot == hm->root) return im;
	return (IMap*) NewHashMap(hm->count + (addedLeaf ? 0 :1), newRoot, hm->hasNull, hm->nullValue);
}

static const IMap* withoutHashMap(const IMap *im, const lisp_object *key) {
	assert(im->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) im;

	if(key == NULL)
		return hm->hasNull ? (IMap*) NewHashMap(hm->count - 1, hm->root, false, NULL) : im;
	if(hm->root == NULL)
		return im;

	const INode *newRoot = hm->root;
	newRoot = newRoot->fns->without(newRoot, 0, HashEq(key), key);

	return (IMap*) NewHashMap(hm->count - 1, newRoot, hm->hasNull, hm->nullValue);
}

static const MapEntry* entryAtHashMap(const IMap *im, const lisp_object *key) {
	assert(im->obj.type == HASHMAP_type);
	const HashMap *hm = (const HashMap*) im;

	if(key == NULL)
		return hm->hasNull ? NewMapEntry(NULL, hm->nullValue) : NULL;

	return hm->root ? hm->root->fns->find(hm->root, 0, HashEq(key), key) : NULL;
}

static const IMap* consHashMap(const IMap *im, const lisp_object *obj) {
	assert(im->obj.type == HASHMAP_type);
	const HashMap *ret = (const HashMap*) im;

	if(isIVector(obj)) {
		const IVector* v = (const IVector*) obj;
		if(v->obj.fns->ICollectionFns->count((ICollection*)v) != 2) {
			exception e = {IllegalArgumentException, "Vector arg to map conj must be a pair"};
			Raise(e);
		}
		return assocHashMap(im, v->obj.fns->IVectorFns->nth(v, 0, NULL), v->obj.fns->IVectorFns->nth(v, 1, NULL));
	}

	for(const ISeq *s = seq(obj); s != NULL; s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *me = (MapEntry*) s->obj.fns->ISeqFns->first(s);
		ret = (HashMap*) assocHashMap((const IMap*) ret, me->key, me->val);
	}

	return (IMap*) ret;
}

static bool EqualsHashMap(const lisp_object *x, const lisp_object *y) {
	assert(x->type == HASHMAP_type);
	const HashMap *hm = (HashMap*)x;
	if(x == y)
		return true;
	if(!isIMap(y))
		return false;
	const IMap *im = (IMap*) y;

	if(countHashMap((ICollection*)hm) != im->obj.fns->ICollectionFns->count((ICollection*)im))
		return false;

	for(const ISeq *s = im->obj.fns->SeqableFns->seq((Seqable*)im); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *e = (MapEntry*) s->obj.fns->ISeqFns->first(s);
		const MapEntry *me = entryAtHashMap((IMap*)hm, e->key);
		if(me == NULL || !Equals(e->val, me->val))
			return false;
	}
	return true;
}
