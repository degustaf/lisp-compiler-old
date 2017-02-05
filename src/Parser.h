#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "LispObject.h"
#include "Lexer.h"

lisp_object *Parser(FILE *input, token *const current);

#endif /* PARSER_H */
