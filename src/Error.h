#ifndef ERROR_H
#define ERROR_H

#include <setjmp.h>
#include <stdbool.h>
#include <stdlib.h>

#include "LispObject.h"

#define MaxExceptions 10
#define ETooManyExceptions 101
#define EUnhandledException 102

#define ES_Initialize 0
#define ES_EvalBody 1
#define ES_Exception 2

typedef enum {	// ExceptionType
	Exception,
	ArityException,
	CompilerException,
	FileNotFoundException,
	IllegalAccessError,
	IllegalArgumentException,
	IllegalStateException,
	IndexOutOfBoundException,
	NumberFormatException,
	ReaderException,
	RuntimeException,
	UnsupportedOperationException
} ExceptionType;

typedef struct {	// exception
	ExceptionType type;
	const char *msg;
} exception;

typedef struct _ctx_block {
	jmp_buf jmp;
	size_t nx;
	const exception *array[MaxExceptions];
	const exception *id;
	bool finally;
	struct _ctx_block *link;
} context_block;

extern const exception ANY;
extern const exception CompilerExcp;
extern const exception ReaderExcp;
extern const exception RuntimeExcp;

extern context_block *exceptionStack;

void Raise(exception e);

#define ReRaise Raise((*(_ctx.id)))

#define TRY \
	{ \
		context_block _ctx; \
		int _es = ES_Initialize; \
		_ctx.nx = 0; \
		_ctx.link = NULL; \
		_ctx.finally = false; \
		_ctx.link = exceptionStack; \
		exceptionStack = &_ctx; \
		if(setjmp(_ctx.jmp)) _es = ES_Exception; \
		while(1) { \
			if(_es == ES_EvalBody) {
#define EXCEPT(e) \
				if(_es == ES_EvalBody) exceptionStack = _ctx.link; \
				break; \
			} \
			if(_es == ES_Initialize) { \
				if(_ctx.nx >= MaxExceptions) \
					exit(ETooManyExceptions); \
				_ctx.array[_ctx.nx++] = &e; \
			} else if(_ctx.id->type == e.type || &e == &ANY) { \
				exceptionStack = _ctx.link;
#define FINALLY \
			} \
			if(_es == ES_Initialize) { \
				if(_ctx.nx >= MaxExceptions) \
					exit(ETooManyExceptions); \
				_ctx.finally = true; \
			} else { \
				exceptionStack = _ctx.link;

#define ENDTRY \
				if(_ctx.finally && _es == ES_Exception) \
					Raise(*_ctx.id); \
				break; \
			} \
			_es = ES_EvalBody; \
		} \
	}

const lisp_object* NewArityError(size_t actual, const char *restrict name);

#endif /* ERROR_H */
