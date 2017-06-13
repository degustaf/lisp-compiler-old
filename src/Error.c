#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"
#include "StringWriter.h"

const exception ANY;
const exception CompilerExcp = {CompilerException, NULL};
const exception ReaderExcp = {ReaderException, NULL};
const exception RuntimeExcp = {RuntimeException, NULL};

context_block *exceptionStack = NULL;

void Raise(exception e) {
	context_block  *xb = exceptionStack;
	for(bool found = false; xb != NULL; xb = xb->link) {
		for(size_t i = 0; i<xb->nx; i++) {
			const exception *t = xb->array[i];
			if(t->type == e.type || t == &ANY) {
				found = true;
				break;
			}
		}
		if(found) break;
	}
	if(xb == NULL)
		fputs(e.msg, stderr);
		exit(EUnhandledException);

	context_block *cb;
	for(cb = exceptionStack; cb != xb && !cb->finally; cb = cb->link);
	exceptionStack = cb;
	exception *e2 = GC_MALLOC(sizeof(e));
	memcpy(e2, &e, sizeof(e));
	cb->id = e2;
	longjmp(cb->jmp, ES_Exception);
}

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

void NewArityError(size_t actual, const char *restrict name) {
	StringWriter *sw = NewStringWriter();
	AddString(sw, "Wrong number of args (");
	AddInt(sw, (int)actual);
	AddString(sw, ") passed to ");
	AddString(sw, name);
	AddChar(sw, '\n');
	exception e = {ArityException, WriteString(sw)};
	Raise(e);
}
