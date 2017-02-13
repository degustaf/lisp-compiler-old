#ifndef LISP_STRINGS_H
#define LISP_STRINGS_H

#include "LispObject.h"

typedef struct char_struct Char;
typedef struct string_struct String;

Char *NewChar(char ch);
String *NewString(char *str);

#endif /* LISP_STRINGS_H */
