#include "Compiler.h"

#include <ctype.h>

#include "Bool.h"
#include "gc.h"
#include "Interfaces.h"
#include "Numbers.h"
#include "Map.h"
#include "Reader.h"
#include "Strings.h"
#include "Symbol.h"
#include "Util.h"
#include "Vector.h"

typedef enum {	// Expr_Context
	STATEMENT,
	EXPRESSION,
	RETURN,
	EVAL
} Expr_Context;

typedef enum {	// expr_type
	NILEXPR_type,
	BOOLEXPR_type,
	NUMBEREXPR_type,
	CONSTANTEXPR_type,
	STRINGEXPR_type,
	EMPTYEXPR_type,
	METAEXPR_type,
	MAPEXPR_type,

	KEYWORDEXPR_type,
} expr_type;

#define EXPR_BASE \
	lisp_object obj; \
	expr_type type; \
	const lisp_object* (*Eval)(const struct Expr_struct*);
// void (*Emit)(const struct Expr_struct*, ...);	// TODO
// const struct Expr_struct* (*parse)(Expr_Context, const lisp_object *form);

typedef struct Expr_struct {
	EXPR_BASE
} Expr;

#define EXPR_OBJECT {EXPR_type, 0, NULL, NULL, NULL, NULL, &NullInterface}

static const Expr* Analyze(Expr_Context context, const lisp_object *form, char *name);
static bool IsLiteralExpr(const Expr *e);
static const Expr* NewStringExpr(const String *form);
static const Expr* NewEmptyExpr(const ICollection *form);

// NilExpr
const lisp_object* EvalNil(__attribute__((unused)) const Expr *self) {
	return NULL;
}
const Expr _NilExpr = {EXPR_OBJECT, NILEXPR_type, EvalNil};
const Expr *const NilExpr = &_NilExpr;

// BoolExpr
const lisp_object* EvalTrue(__attribute__((unused)) const Expr *self) {
	return (lisp_object*)True;
}
const Expr _TrueExpr = {EXPR_OBJECT, BOOLEXPR_type, EvalTrue};
const Expr *const TrueExpr = &_TrueExpr;

const lisp_object* EvalFalse(__attribute__((unused)) const Expr *self) {
	return (lisp_object*)False;
}
const Expr _FalseExpr = {EXPR_OBJECT, BOOLEXPR_type, EvalFalse};
const Expr *const FalseExpr = &_FalseExpr;

// NumberExpr
typedef struct {	// NumberExpr
	EXPR_BASE
	const Number *n;
	// const size_t id;
} NumberExpr;

static const lisp_object* EvalNumber(const Expr *self){
	assert(self->type == NUMBEREXPR_type);
	return (lisp_object*) ((NumberExpr*)self)->n;
}

static const Expr* parseNumber(const Number *form) {
	NumberExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = NUMBEREXPR_type;
	ret->Eval = EvalNumber;
	ret->n = form;

	return (Expr*) ret;
}

// ConstantExpr
typedef struct {	// ConstantExpr
	EXPR_BASE
	const lisp_object *x;
	// const size_t id;
} ConstantExpr;

static const lisp_object* EvalConstant(const Expr *self){
	assert(self->type == CONSTANTEXPR_type);
	return (lisp_object*) ((ConstantExpr*)self)->x;
}

static const Expr* NewConstantExpr(const lisp_object *v) {
	ConstantExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = CONSTANTEXPR_type;
	ret->Eval = EvalConstant;
	ret->x = v;

	return (Expr*)ret;
}

static const Expr* parseConstant(const lisp_object *form) {
	size_t argcount = count(form) - 1;
	if(argcount != 1) {
		// TODO throw exception
	}

	const ISeq *s = seq(form);
	s = s->obj.fns->ISeqFns->next(s);
	const lisp_object *v = s->obj.fns->ISeqFns->first(s);

	if(v == NULL)
		return NilExpr;
	if(v == (void*)True)
		return TrueExpr;
	if(v == (void*)False)
		return FalseExpr;
	if(isNumber(v))
		return parseNumber((Number*)v);
	if(v->type == STRING_type)
		return NewStringExpr((String*)v);
	if(isICollection(v) && v->fns->ICollectionFns->count((ICollection*)v) == 0)
		return NewEmptyExpr((ICollection*)v);
	return NewConstantExpr(v);
}

// StringExpr
typedef struct {	// StringExpr
	EXPR_BASE
	const String *s;
} StringExpr;

static const lisp_object* EvalString(const Expr *self){
	assert(self->type == STRINGEXPR_type);
	return (lisp_object*) ((StringExpr*)self)->s;
}

static const Expr* NewStringExpr(const String *form) {
	StringExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = STRINGEXPR_type;
	ret->Eval = EvalString;
	ret->s = form;

	return (Expr*) ret;
}

// EmptyExpr
typedef struct {	// EmptyExpr
	EXPR_BASE
	const ICollection *coll;
} EmptyExpr;

static const lisp_object* EvalEmpty(const Expr *self){
	assert(self->type == EMPTYEXPR_type);
	return (lisp_object*) ((EmptyExpr*)self)->coll;
}

static const Expr* NewEmptyExpr(const ICollection *form) {
	EmptyExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = EMPTYEXPR_type;
	ret->Eval = EvalEmpty;
	ret->coll = form;

	return (Expr*) ret;
}

// MetaExpr
typedef struct {	// MetaExpr
	EXPR_BASE
	const Expr *x;
	const Expr *meta;
} MetaExpr;

static const lisp_object* EvalMeta(const Expr *self){
	assert(self->type == METAEXPR_type);
	const MetaExpr *m = (MetaExpr*)self;
	return withMeta(m->x->Eval(m->x), (IMap*)m->meta->Eval(m->meta));
}

static const Expr* NewMetaExpr(const Expr *x, const Expr *meta) {
	MetaExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = METAEXPR_type;
	ret->Eval = EvalMeta;
	ret->x = x;
	ret->meta = meta;

	return (Expr*) ret;
}

// MapExpr
typedef struct {	// MapExpr
	EXPR_BASE
	const IVector *keyVals;
} MapExpr;

static const lisp_object* EvalMap(const Expr *self){
	assert(self->type == MAPEXPR_type);
	const MapExpr *m = (MapExpr*)self;
	size_t count = m->keyVals->obj.fns->ICollectionFns->count((ICollection*)m->keyVals);
	const lisp_object* entries[count];
	for(size_t i = 0; i < count; i++) {
		entries[i] = m->keyVals->obj.fns->IVectorFns->nth(m->keyVals, i, NULL);
	}
	return (lisp_object*)CreateHashMap(count, entries);
}

static const Expr* parseMapExpr(Expr_Context context, const IMap *form) {
	const IVector *keyVals = (IVector*)EmptyVector;
	bool keysConstant = true;
	bool valsConstant = true;
	bool allConstantKeysUnique = true;
	const IMap *constantKeys = (IMap*) EmptyHashMap;
	for(const ISeq *s = form->obj.fns->SeqableFns->seq((Seqable*)form); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		MapEntry *e = (MapEntry*)s->obj.fns->ISeqFns->first(s);
		const Expr *key = Analyze(context == EVAL ? EVAL : EXPRESSION, e->key, NULL);
		const Expr *val = Analyze(context == EVAL ? EVAL : EXPRESSION, e->val, NULL);
		keyVals = keyVals->obj.fns->IVectorFns->cons(keyVals, (lisp_object*)key);
		keyVals = keyVals->obj.fns->IVectorFns->cons(keyVals, (lisp_object*)val);

		if(IsLiteralExpr(key)) {
			const lisp_object *k = key->Eval(key);
			if(constantKeys->obj.fns->IMapFns->entryAt(constantKeys, k)) {
				allConstantKeysUnique = false;
			} else {
				constantKeys = constantKeys->obj.fns->IMapFns->assoc(constantKeys, k, k);
			}
		} else {
			keysConstant = false;
		}
		if(!IsLiteralExpr(val))
			valsConstant = false;
	}

	MapExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->keyVals = keyVals;
	ret->Eval = EvalMap;

	if(form->obj.meta) {
		return NewMetaExpr((Expr*)ret, parseMapExpr(context == EVAL ? EVAL : EXPRESSION, form->obj.meta));
	}
	if(keysConstant) {
		if(!allConstantKeysUnique) {
			// TODO throw Exception.
		}
		if(valsConstant) {
			const IMap *m = (IMap*)EmptyHashMap;
			for(size_t i = 0; i < keyVals->obj.fns->ICollectionFns->count((ICollection*)keyVals); i+=2) {
				Expr *k = (Expr*)keyVals->obj.fns->IVectorFns->nth(keyVals, i, NULL);
				Expr *v = (Expr*)keyVals->obj.fns->IVectorFns->nth(keyVals, i+1, NULL);
				m = m->obj.fns->IMapFns->assoc(m, k->Eval(k), v->Eval(v));
			}
			return NewConstantExpr((lisp_object*)m);
		}
	}

	return (Expr*)ret;
}




static void consumeWhitespace(FILE *input) {
	int ch = getc(input);
	while(isspace(ch))
		ch = getc(input);
	ungetc(ch, input);
}

static const lisp_object* macroExpand1(const lisp_object *form) {
	// TODO macroExpand1
	return form;
}

static const lisp_object* macroExpand(const lisp_object *form) {
	const lisp_object *exp = macroExpand1(form);
	if(exp != form) 
		return macroExpand1(exp);
	return form;
}

static const Expr* AnalyzeSeq(Expr_Context context, const lisp_object *form, char *name) {
	const lisp_object *me = macroExpand1(form);
	if(me != form)
		return Analyze(context, me, name);
	// TODO
}

static const Expr* Analyze(Expr_Context context, const lisp_object *form, char *name) {
	// TODO if form is LazySeq, convert form to Seq.
	if(form == NULL) {
		return NilExpr;
	}
	if(form == (void*)True) {
		return TrueExpr;
	}
	if(form == (void*)False) {
		return FalseExpr;
	}
	if(form->type == SYMBOL_type) {
		// TODO analyzeSymbol
	}
	if(form->type == KEYWORD_type) {
		// TODO registerKeyword
	}
	if(isNumber(form)) {
		return parseNumber((Number*)form);
	}
	if(form->type == STRING_type) {
		return NewStringExpr((String*)form);
	}
	if(isICollection(form) /* && not IRecord && not IType */ && form->fns->ICollectionFns->count((ICollection*)form) == 0) {
		const Expr *ret = NewEmptyExpr((ICollection*)form);
		if(form->meta) {
			ret = NewMetaExpr(ret, parseMapExpr(context == EVAL ? EVAL : EXPRESSION, form->meta));
		}
		return ret;
	}
	if(isISeq(form)) {
		// TODO return AnalyzeSeq(...
	}
	if(isIVector(form)) {
		// TODO parseVectorExpr(...
	}
	// TODO IRecord
	// TODO IType
	if(isIMap(form)) {
		return parseMapExpr(context, (IMap*)form);
	}
	// TODO ISet
	return NewConstantExpr(form);
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
	const Expr *x = Analyze(EVAL, form, NULL);
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

static bool IsLiteralExpr(const Expr *e) {
	return e->type == KEYWORDEXPR_type || e->type == NUMBEREXPR_type || e->type == CONSTANTEXPR_type ||
			e->type == NILEXPR_type || e->type == BOOLEXPR_type || e->type == STRINGEXPR_type;
}
