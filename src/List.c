#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "List.h"

char *EmptyListToString(lisp_object *obj);
LLVMValueRef List_codegen(struct lisp_object_struct *obj);
lisp_object *ListCopy(lisp_object *obj);

struct List_struct {
    lisp_object obj;
    const lisp_object *const _first;
    const struct List_struct *const _rest;
    const size_t _count;
};
    
const List _EmptyList = {{LIST_TYPE, &List_codegen, &EmptyListToString, &ListCopy}, NULL, NULL, 0};
const List *const EmptyList = &_EmptyList;
const char *const EmptyListString = "()";

LLVMValueRef List_codegen(struct lisp_object_struct *obj) {
    return NULL;
}

char *EmptyListToString(lisp_object *obj) {
    return (char*)EmptyListString;
}

char *ListToString(lisp_object *obj) {
    assert(obj->type == LIST_TYPE);
    size_t buffer_size = 256;
    char *buffer = malloc(buffer_size * sizeof(*buffer));
    size_t size = 1;

    buffer[0] = '(';
    buffer[1] = '\0';

    for(const List *list = (List*)obj; list != EmptyList; list = list->_rest) {
        const lisp_object *o = list->_first;
        char *str = o->toString((lisp_object*)o);
        size_t len = strlen(str);
        size_t growth;
        for(growth = 1; size + len + 2 >= growth * buffer_size; growth *= 2);
        if(growth > 1) {
            buffer_size *= growth;
            buffer = realloc(buffer, buffer_size * sizeof(*buffer));
        }
        strncpy(buffer, str, len+1);
        buffer[size++] = ' ';
        size += len + 1;
    }
    buffer[size++] = ')';
    buffer[size] = '\0';
    return buffer;
}

lisp_object *ListCopy(lisp_object *obj) {
    assert(obj->type == LIST_TYPE);
    List *list = (List*)obj;
    size_t count = list->_count;
    lisp_object **arr = malloc(count * sizeof(*arr));
    for(size_t i=0; i<count; i++) {
        const lisp_object *const o = list->_first;
        list = (List*)list->_rest;
        arr[i] = (*(o->copy))((lisp_object*)o);
    }
    return (lisp_object*)CreateList(count, arr);
}

const List *NewList(const lisp_object *const first) {
    List *ret = malloc(sizeof(*ret));
    if(ret == NULL) return NULL;

    List _ret = {{LIST_TYPE, &List_codegen, &ListToString, &ListCopy},
                 first,
                 EmptyList,
                 1};
    memcpy(ret, &_ret, sizeof(*ret));

    return ret;
}

const List *CreateList(size_t count, lisp_object **entries) {
    List *ret = (List*)EmptyList;
    for(size_t i = 1; i <= count; i++) {
        List _ret = {{LIST_TYPE, &List_codegen, &ListToString, &ListCopy},
                     entries[count-i],
                     ret,
                     i};
        ret = malloc(sizeof(*ret));
        memcpy(ret, &_ret, sizeof(*ret));
    }
    return ret;
}
