#include "Error.h"

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
