#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"

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
	Error *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EOFflag ? EOF_type : ERROR_type;
	ret->obj.toString = toStringError;
	ret->obj.fns = &NullInterface;

	va_list argp, argp2;
	va_start(argp, format);
	va_copy(argp2, argp);
	size_t len = vsnprintf(NULL, 0, format, argp);
	ret->msg = GC_MALLOC((len+1) * sizeof(*format));
	vsnprintf(ret->msg, len+1, format, argp2);
	va_end(argp);
	va_end(argp2);

	return ret;
}

const Error *NewArityError(size_t actual, const char *restrict name) {
	return NewError(false, "Wrong number of args (%zd) passed to %s\n", actual, name);
}
