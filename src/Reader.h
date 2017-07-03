#ifndef READER_H
#define READER_H

#include "LispObject.h"

#include <stdbool.h>

#include "LineNumberReader.h"

void init_reader();
const lisp_object *read(LineNumberReader *input, bool EOF_is_error, char return_on);

#endif /* READER_H */
