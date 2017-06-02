#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#include "LispObject.h"

typedef struct ErrorStruct Error;

const Error *NewError(bool EOFflag, const char *restrict format, ...);

const Error *NewArityError(size_t actual, const char *restrict name);

#endif /* ERROR_H */
