// #include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "List.h"

LLVMValueRef List_codegen(struct lisp_object_struct *obj);

struct List_struct {
    lisp_object obj;
    const lisp_object *const _first;
    const struct List_struct *const _rest;
    const size_t _count;
};
    
const List _EmptyList = {{LIST_TYPE, &List_codegen, NULL}, NULL, NULL, 0};
const List *const EmptyList = &_EmptyList;

LLVMValueRef List_codegen(struct lisp_object_struct *obj) {
    return NULL;
}

const List *NewList(const lisp_object *const first) {
    List *ret = malloc(sizeof(*ret));
    if(ret == NULL) return NULL;

    List _ret = {{LIST_TYPE, &List_codegen, NULL}, first, EmptyList, 1};
    memcpy(ret, &_ret, sizeof(*ret));

    return ret;
}

const List *CreateList(lisp_object **entries, size_t count) {
    List *ret = (List*)EmptyList;
    for(size_t i = 1; i <= count; i++) {
        List _ret = {{LIST_TYPE, &List_codegen, NULL},
                     entries[count-i],
                     ret,
                     i};
        ret = malloc(sizeof(*ret));
        memcpy(ret, &_ret, sizeof(*ret));
    }
    return ret;
}
