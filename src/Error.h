#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>

#include "LispObject.h"

typedef struct ErrorStruct Error;

const Error *NewError(bool EOFflag, const char *restrict format, ...);

#endif /* ERROR_H */
