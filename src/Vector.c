#include <stdbool.h>
#include <stddef.h>     // Can be removed if stdlib.h is added.

#include "Vector.h"

#define LOG_NODE_SIZE 5
#define NODE_SIZE (1 << LOG_NODE_SIZE)
#define BITMASK (NODE_SIZE - 1)

typedef struct {
    bool editable;
    pthread_t thread_id;
    lisp_object *array[NODE_SIZE];
} Node;

const Node _EmptyNode = {false, 0, {NULL}};
const Node *const EmptyNode = &_EmptyNode;


struct Vector_struct {
    lisp_object obj;
    size_t count;
};

const Vector *const EmptyVector;

const Vector *CreateVector(size_t count, lisp_object **entries) {
}
