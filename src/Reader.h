#ifndef READER_H
#define READER_H

#include <stdbool.h>
#include <stdio.h>

#include "LispObject.h"

void init_reader();
const lisp_object *read(FILE *input, bool EOF_is_error, char return_on);

#endif /* READER_H */
