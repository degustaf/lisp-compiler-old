#include "Error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct ErrorStruct {
	lisp_object obj;
	char *msg;
};

static const char *toStringError(const lisp_object *obj) {
	assert(obj->type == ERROR_type || obj->type == EOF_type);
	const Error *e = (const Error*) obj;
	return e->msg;
}

const Error *NewError(bool EOFflag, const char *restrict format, ...) {
	Error *ret = malloc(sizeof(*ret));
	ret->obj.type = EOFflag ? EOF_type : ERROR_type;
	ret->obj.codegen = NULL;
	ret->obj.toString = toStringError;
	ret->obj.fns = &NullInterface;

	va_list argp, argp2;
	va_start(argp, format);
	va_copy(argp2, argp);
	size_t len = vsnprintf(NULL, 0, format, argp);
	ret->msg = calloc((len+1), sizeof(*format));
	vsnprintf(ret->msg, len+1, format, argp2);
	va_end(argp);
	va_end(argp2);

	return ret;
}
