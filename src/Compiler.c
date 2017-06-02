#include "Compiler.h"

#include <ctype.h>

#include "Interfaces.h"
#include "Reader.h"
#include "Strings.h"
#include "Symbol.h"
#include "Util.h"

typedef enum {
	STATEMENT,
	EXPRESSION,
	RETURN,
	EVAL
} Expr_Context;

typedef struct Expr_struct {
	const lisp_object* (*Eval)(const struct Expr_struct*);
	// void (*Emit)(const struct Expr_struct*, ...);	// TODO
	const struct Expr_struct* (*parse)(Expr_Context, const lisp_object *form);
} Expr;

static void consumeWhitespace(FILE *input) {
	int ch = getc(input);
	while(isspace(ch))
		ch = getc(input);
	ungetc(ch, input);
}

static const lisp_object* macroExpand(const lisp_object *form) {
	// TODO macroExpand
	return form;
}

static const Expr* Analyze(Expr_Context context, const lisp_object *form) {
	// TODO Analyze
	return NULL;
}

static const lisp_object* Eval(const lisp_object *form) {
	// https://github.com/clojure/clojure/blob/master/src/jvm/clojure/lang/Compiler.java#L6971
	form = macroExpand(form);
	if(isISeq(form)) {
		const ISeq *s = (ISeq*)form;
		if(Equals((lisp_object*)DoSymbol, s->obj.fns->ISeqFns->first(s))) {
			for(s = s->obj.fns->ISeqFns->next(s); s->obj.fns->ISeqFns->next(s) != NULL; s = s->obj.fns->ISeqFns->next(s)) {
				Eval(s->obj.fns->ISeqFns->first(s));
			}
			return Eval(s->obj.fns->ISeqFns->first(s));
		}
	}
	const Expr *x = Analyze(EVAL, form);
	return x->Eval(x);
}

const lisp_object* compilerLoad(FILE *reader, __attribute__((unused)) const char *path, __attribute__((unused)) const char *name) {
	const lisp_object *ret = NULL;
	// const lisp_object *mapArgs[] = {(const lisp_object*)NewString(path), (const lisp_object*)NewString(name)};
	// size_t mapArgc = sizeof(mapArgs)/sizeof(mapArgs[0]);
	// TODO pushThreadBindings
	// try
	for(const lisp_object *r = read(reader, true, '\0'); r->type != EOF_type; r = read(reader, true, '\0')) {
		// TODO
		ret = Eval(r);
	}
	// TODO
	return ret;
}
