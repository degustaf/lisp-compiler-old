#ifndef LISP_OBJECT_H
#define LISP_OBJECT_H

#include "llvm-c/Types.h"

typedef enum {
    INTEGER_type,
    FLOAT_type,
} object_type;

typedef struct lisp_object_struct {
    object_type type;
    LLVMValueRef (*codegen)(struct lisp_object_struct *);
} lisp_object;

#endif /* LISP_OBJECT_H */
