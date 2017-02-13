#ifndef LISP_OBJECT_H
#define LISP_OBJECT_H

#include "llvm-c/Types.h"

typedef enum {
    EOF_type,
    DONE_type,
    NOOP_type,
    CHAR_TYPE,
    STRING_TYPE,
    INTEGER_type,
    FLOAT_type,
    LIST_TYPE,
} object_type;

typedef struct lisp_object_struct {
    object_type type;
    LLVMValueRef (*codegen)(struct lisp_object_struct *);
    char* (*toString)(struct lisp_object_struct *);
} lisp_object;

#endif /* LISP_OBJECT_H */
