#include "Compiler.h"

#include <ctype.h>
#include <string.h>

#include "Bool.h"
#include "gc.h"
#include "Interfaces.h"
#include "Keyword.h"
#include "List.h"
#include "Numbers.h"
#include "Map.h"
#include "Reader.h"
#include "RunTime.h"
#include "Strings.h"
#include "Symbol.h"
#include "Util.h"
#include "Vector.h"

Var *ALLOW_UNRESOLVED_VARS = NULL;
Var *CLEAR_PATH = NULL;
Var *CLEAR_ROOT = NULL;
Var *CLEAR_SITES = NULL;
Var *COMPILER_OPTIONS = NULL;
Var *COMPILE_STUB_SYM = NULL;
Var *COMPILE_STUB_CLASS = NULL;
Var *CONSTANTS = NULL;
Var *CONSTANT_IDS = NULL;
Var *IN_CATCH_FINALLY = NULL;
Var *INSTANCE = NULL;
Var *KEYWORD_CALLSITES = NULL;
Var *KEYWORDS = NULL;
Var *METHOD = NULL;
Var *METHOD_RETURN_CONTEXT = NULL;
Var *LOCAL_ENV = NULL;
Var *SOURCE = NULL;
Var *VARS = NULL;

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
	KWINVOKEEXPR_type,
	INVOKEEXPR_type,
	VAREXPR_type,
	LOCALBINDINGEXPR_type,
	OBJEXPR_type,
	VECTOREXPR_type,
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

typedef enum {	// PATHTYPE
	PATH,
	BRANCH
} PATHTYPE;

#define EXPR_OBJECT {EXPR_type, 0, NULL, NULL, NULL, NULL, &NullInterface}

static int registerConstant(const lisp_object *obj);
static int registerKeywordCallsite(const Keyword *k);
static const Expr* Analyze(Expr_Context context, const lisp_object *form, char *name);
static bool IsLiteralExpr(const Expr *e);
static const Expr* NewStringExpr(const String *form);
static const Expr* NewEmptyExpr(const ICollection *form);
static const Symbol* tagOf(const lisp_object *o);
static bool inTailCall(Expr_Context C);
static const char* munge(const char *name);
static const char* demunge(const char *mungedName);
static const object_type* maybePrimitiveType(const Expr *e);
static const lisp_object* resolveIn(Namespace *ns, const Symbol *sym, bool allowPrivate);

// PathNode
typedef struct PathNode_struct {
	lisp_object *obj;
	PATHTYPE type;
	const struct PathNode_struct *parent;
} PathNode;

static const ISeq* fwdPath(const PathNode *p) {
	const ISeq *ret = (ISeq*) EmptyList;
	for(; p != NULL; p = p->parent) {
		ret = ret->obj.fns->ISeqFns->cons(ret, (lisp_object*)p);
	}
	return ret;
}

static const PathNode* commonPath(const PathNode *n1, const PathNode *n2) {
	const ISeq *s1 = fwdPath(n1);
	const ISeq *s2 = fwdPath(n2);
	if(s1->obj.fns->ISeqFns->first(s1) != s2->obj.fns->ISeqFns->first(s2))
		return NULL;
	while(second((lisp_object*)s1) != NULL && second((lisp_object*)s1) == second((lisp_object*)s2)) {
		s1 = s1->obj.fns->ISeqFns->next(s1);
		s2 = s2->obj.fns->ISeqFns->next(s2);
	}
	return (PathNode*) s1->obj.fns->ISeqFns->first(s1);
}

typedef struct ObjExpr_struct ObjExpr;

// ObjMethod
typedef struct ObjMethod_struct { // ObjMethod
	lisp_object obj;
	struct ObjMethod_struct *parent;
	const IMap *locals;
	const IMap *indexLocals;
	const Expr *body;
	const ObjExpr *objx;
	const IVector argLocals;
	size_t maxLocal;
	size_t line;
	size_t column;
	bool useThis;
	const IMap /* TODO ISet */ *localsUsedInCatchFinally;
	const IMap *methodMeta;
} ObjMethod;

const ObjMethod* NewObjMethod(const ObjExpr *objx, ObjMethod *parent) {
	ObjMethod *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = OBJMETHOD_type;
	ret->obj.fns = &NullInterface;
	ret->parent = parent;
	ret->objx = objx;
	ret->localsUsedInCatchFinally = (IMap*) EmptyHashMap;	// EmptyHashSet

	return ret;
}

// ObjExpr
struct ObjExpr_struct {
	EXPR_BASE
	const char *name;
	const char *internalName;
	const char *thisName;
	// Type objType;
	const lisp_object *tag;
	const IMap *closes;
	const IMap *closesExpr;
	const IMap *volatiles;
	const IMap *fields;
	const IVector *hintedFields;
	const IMap *keywords;
	const IMap *vars;
	// Class compiledClass;
	// int line;
	// int column;
	const IVector *constants;
	const IMap /* ISet */ *usedConstants;	// TODO
	int constantsID;
	int altCtorDrops;
	const IVector *keywordCallsites;
	const IVector *protocolCallsites;
	const IMap /* ISet */ *varCallsites;	// TODO
	bool onceOnly;
	const lisp_object *src;
	const IMap *opts;
	const IMap *classMeta;
	bool canBeDirect;
};

const ObjExpr* NewObjExpr(const lisp_object *tag) {
	ObjExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = OBJEXPR_type;
	// ret->Eval = ;
	ret->tag = tag;
	ret->closes = (IMap*) EmptyHashMap;
	ret->closesExpr = (IMap*) EmptyHashMap;
	ret->volatiles = (IMap*) EmptyHashMap;
	ret->hintedFields = (IVector*) EmptyVector;
	ret->keywords = (IMap*) EmptyHashMap;
	ret->vars = (IMap*) EmptyHashMap;
	ret->usedConstants = (IMap*) EmptyHashMap;
	// ret->usedConstants = (ISet*) EmptyHashSet;	// TODO
	ret->onceOnly = false;
	ret->opts = (IMap*) EmptyHashMap;

	return ret;
}

// LocalBinding
typedef struct {	// LocalBinding
	const Symbol *sym;
	const Symbol *tag;
	const Expr *init;
	size_t index;
	const char *name;
	bool isArg;
	const PathNode *clearPathRoot;
	bool canBeCleared;
	bool recurMismatch;
	bool used;
} LocalBinding;

__attribute__((unused)) static LocalBinding* NewLocalBinding(int num, const Symbol *sym, const Symbol *tag, const Expr *init, bool isArg, const PathNode *clearPathRoot) {
	LocalBinding *ret = GC_MALLOC(sizeof(*ret));
	if(maybePrimitiveType(init) && tag) {
		// 	throw new UnsupportedOperationException("Can't type hint a local with a primitive initializer");
	}
	ret->index = num;
	ret->sym = sym;
	ret->tag = tag;
	ret->init = init;
	ret->isArg = isArg;
	ret->clearPathRoot = clearPathRoot;
	ret->name = munge(getNameSymbol(sym));
	// ret->canBeCleared = !RT.booleanCast(getCompilerOption(disableLocalsClearingKey));
	ret->canBeCleared = false;
	ret->recurMismatch = false;
	ret->used = false;

	return ret;
}

static const object_type* getPrimitiveType(const LocalBinding *b) {
	return maybePrimitiveType(b->init);
}

static void closeOver(const LocalBinding *b, ObjMethod *method) {
	if(b && method) {
		const LocalBinding *lb = (LocalBinding*) get((lisp_object*)method->locals, (lisp_object*)b, NULL);
		if(lb) {
			if(lb->index == 0)
				method->useThis = true;
			if(deref(IN_CATCH_FINALLY)) {
				const IMap *m = method->localsUsedInCatchFinally;
				method->localsUsedInCatchFinally = m->obj.fns->IMapFns->assoc(m, (lisp_object*)b->index, (lisp_object*)True);
			}
		} else {
			// method->objx->closes = assoc(method->objx->closes, (lisp_object*)b, (lisp_object*)b);	// TODO requires ObjExpr
			closeOver(b, method->parent);
		}
	}
}

static LocalBinding* referenceLocal(const Symbol *sym) {
	if(!isBound(LOCAL_ENV))
		return NULL;
	LocalBinding *b = (LocalBinding*) get(deref(LOCAL_ENV), (lisp_object*)sym, NULL);

	if(b) {
		ObjMethod *method = (ObjMethod*)deref(METHOD);
		if(b->index == 0)
			method->useThis = true;
		closeOver(b, method);
	}

	return b;
}

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
	int id;
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
	ret->id = registerConstant((lisp_object*)form);

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

static const Expr* parseConstant(__attribute__((unused)) Expr_Context context, const lisp_object *form) {
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

// KeywordExpr
typedef struct {	// KeywordExpr
	EXPR_BASE
	const Keyword *k;
} KeywordExpr;

static const lisp_object* EvalKeyword(const Expr *self){
	assert(self->type == KEYWORDEXPR_type);
	return (lisp_object*) ((KeywordExpr*)self)->k;
}

static const Expr* NewKeywordExpr(const Keyword* k) {
	KeywordExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = KEYWORDEXPR_type;
	ret->Eval = EvalKeyword;
	ret->k = k;

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

// KeywordInvokeExpr
typedef struct {	// KeywordInvokeExpr
	EXPR_BASE
	const KeywordExpr *kw;
	const lisp_object *tag;
	const Expr *target;
	int line;
	int column;
	int siteIndex;
	const char *source;
} KeywordInvokeExpr;

static const lisp_object* EvalKwInvoke(const Expr *self) {
	assert(self->type == KEYWORDEXPR_type);
	const KeywordInvokeExpr *KWI = (KeywordInvokeExpr*)self;
	return ((lisp_object*)KWI->kw->k)->fns->IFnFns->invoke1((IFn*)KWI->kw->k, KWI->target->Eval(KWI->target));
}

static Expr* NewKeyWordInvokeExpr(const char *source, const Symbol *tag, const KeywordExpr *kw, const Expr *target) {
	KeywordInvokeExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = KWINVOKEEXPR_type;
	ret->Eval = EvalKwInvoke;

	ret->source = source;
	ret->kw = kw;
	ret->target = target;
	ret->tag = (lisp_object*)tag;
	ret->siteIndex = registerKeywordCallsite(kw->k);

	return (Expr*) ret;
}

// VarExpr
typedef struct {	// VarExpr
	EXPR_BASE
	const Var *v;
	const lisp_object *tag;
} VarExpr;

static const lisp_object* EvalVar(const Expr *self) {
	assert(self->type == VAREXPR_type);
	const VarExpr *ve = (VarExpr*)self;
	return deref(ve->v);
}

static const Expr* NewVarExpr(const Var *v, const Symbol *tag) {
	VarExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = VAREXPR_type;
	ret->Eval = EvalVar;

	ret->v = v;
	ret->tag = tag ? (lisp_object*) tag : getTag(v);

	return (Expr*) ret;
}

// InvokeExpr
typedef struct {	// InvokeExpr
	EXPR_BASE
	const Expr *fexpr;
	const lisp_object *tag;
	const IVector *args;
	// int line;
	// int column;
	bool tailPosition;
	const char *source;
	bool isProtocol;
	bool isDirect;
	int siteIndex;
	// Class protocolOn;
	// public java.lang.reflect.Method onMethod;
} InvokeExpr;

static const lisp_object* EvalInvoke(const Expr *self) {
	assert(self->type == INVOKEEXPR_type);
	const InvokeExpr *Invk = (InvokeExpr*)self;
	// TRY
	IFn *f = (IFn*) Invk->fexpr->Eval(Invk->fexpr);
	const IVector *args = (IVector*)EmptyVector;
	for(size_t i = 0; i < Invk->args->obj.fns->ICollectionFns->count((ICollection*)Invk->args); i++) {
		Expr *ith = (Expr*)Invk->args->obj.fns->IVectorFns->nth(Invk->args, i, NULL);
		args = args->obj.fns->IVectorFns->cons(args, ith->Eval(ith));
	}
	return f->obj.fns->IFnFns->applyTo(f, args->obj.fns->SeqableFns->seq((Seqable*)args));
	// CATCH
}

static const lisp_object* sigTag(size_t argCount, const Var *v) {
	const IMap *meta = ((lisp_object*)v)->meta;
	const lisp_object *arglists = meta->obj.fns->IMapFns->entryAt(meta, (lisp_object*)arglistsKW);

	for(const ISeq *s = seq(arglists); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const Vector *sig = (Vector*)s->obj.fns->ISeqFns->first(s);
		int restOffset = indexOf((IVector*)sig, (lisp_object*)AmpSymbol);
		if(argCount == ((lisp_object*)sig)->fns->ICollectionFns->count((ICollection*)sig) || (restOffset > -1 && (int)argCount >= restOffset))
			return (lisp_object*) tagOf((lisp_object*)sig);
	}
	return NULL;
}

static Expr* NewInvokeExpr(const char *source, /* int line, int column, */ const Symbol *tag, const Expr *fexpr, const IVector *args, bool tailPosition) {
	InvokeExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = INVOKEEXPR_type;
	ret->Eval = EvalInvoke;

	ret->source = source;
	ret->fexpr = fexpr;
	ret->args = args;
	// ret->line = line;
	// ret->column = column;
	ret->tailPosition = tailPosition;

	if(fexpr->type == VAREXPR_type) {
		// TODO This is for protocols.
	}

	if(tag) {
		ret->tag = (lisp_object*)tag;
	} else if(fexpr->type == VAREXPR_type) {
		const Var *v = ((VarExpr*)fexpr)->v;
		const lisp_object *sig = sigTag(args->obj.fns->ICollectionFns->count((ICollection*)args), v);
		ret->tag = sig ? sig : ((VarExpr*)fexpr)->tag;
	} else {
		ret->tag = NULL;
	}

	return (Expr*)ret;
}

static Expr* parseInvokeExpr(Expr_Context context, const ISeq *form) {
	bool tailPosition = inTailCall(context);
	if(context != EVAL)
		context = EXPRESSION;
	const Expr *fexpr = Analyze(context, form->obj.fns->ISeqFns->first(form), NULL);
	if((fexpr->type == VAREXPR_type) && Equals((lisp_object*)((VarExpr*)fexpr)->v, (lisp_object*)INSTANCE)
			&& (form->obj.fns->ICollectionFns->count((ICollection*)form) == 3)) {
		const Expr *SecExpr = Analyze(EXPRESSION, second((lisp_object*)form), NULL);
		if(SecExpr->type == CONSTANTEXPR_type) {
			// TODO This requires solving how to handle classes.
			// const lisp_object *val = SecExpr->Eval(SecExpr);
			// if(val instanceof Class)
			// 	return NewInstanceOfExpr(...);
		}
	}

	// TODO direct linking

	if((fexpr->type == VAREXPR_type) && (context != EVAL)) {
		const Var *v = ((VarExpr*)fexpr)->v;
		const IMap *vMeta = ((lisp_object*)v)->meta;
		const lisp_object *arglist = vMeta->obj.fns->IMapFns->entryAt(vMeta, (lisp_object*)arglistsKW);
		size_t arity = count((lisp_object*)form->obj.fns->ISeqFns->next(form));
		for(const ISeq *s = seq(arglist); s != NULL; s->obj.fns->ISeqFns->next(s)) {
			const IVector *args = (IVector*) s->obj.fns->ISeqFns->first(s);
			if(args->obj.fns->ICollectionFns->count((ICollection*)args) == arity) {
				__attribute__((unused)) char *primc = NULL;	// TODO requires FnMethod
				// return Analyze(context, listStar3(internSymbol1(".invokePrim"),
			}
		}
	}

	if((fexpr->type == KEYWORDEXPR_type) && (form->obj.fns->ICollectionFns->count((ICollection*)form) == 2) && isBound(KEYWORD_CALLSITES)) {
		const Expr *target = Analyze(context, second((lisp_object*)form), NULL);
		return NewKeyWordInvokeExpr(toString(deref(SOURCE)), tagOf((lisp_object*)form), (KeywordExpr*)fexpr, target);
	}

	const IVector *args = (IVector*) EmptyVector;
	for(const ISeq *s = form->obj.fns->ISeqFns->next(form); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		args = args->obj.fns->IVectorFns->cons(args, (lisp_object*)Analyze(context, s->obj.fns->ISeqFns->first(s), NULL));
	}

	return NewInvokeExpr(toString(deref(SOURCE)), tagOf((lisp_object*)form), fexpr, args, tailPosition);
}

// LocalBindingExpr
typedef struct {	// LocalBindingExpr
	EXPR_BASE
	const LocalBinding *b;
	const Symbol *tag;
	const PathNode *clearPath;
	const PathNode *clearRoot;
	bool shouldClear;
} LocalBindingExpr;

static const lisp_object* EvalLocalBinding(__attribute__((unused)) const Expr *self) {
	// throw new UnsupportedOperationException("Can't eval locals");
	return NULL;
}

static Expr* NewLocalBindingExpr(LocalBinding *b, const Symbol *tag) {
	if(getPrimitiveType(b) && tag) {
		// throw new UnsupportedOperationException("Can't type hint a primitive local");
	}
	LocalBindingExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = LOCALBINDINGEXPR_type;
	ret->Eval = EvalLocalBinding;

	ret->b = b;
	ret->tag = tag;
	ret->clearPath = (PathNode*) getVar(CLEAR_PATH);
	ret->clearRoot = (PathNode*) getVar(CLEAR_ROOT);
	const ICollection *sites = (ICollection*) get(getVar(CLEAR_SITES), (lisp_object*)b, NULL);
	b->used = true;

	if(b->index > 0) {
		if(sites) {
			for(const ISeq *s = sites->obj.fns->SeqableFns->seq((Seqable*)sites); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
				LocalBindingExpr *o = (LocalBindingExpr*) s->obj.fns->ISeqFns->first(s);
				const PathNode *common = commonPath(ret->clearPath, o->clearPath);
				if(common && common->type == PATH)
					o->shouldClear = false;
			}
		}
	}

	if(ret->clearRoot == b->clearPathRoot) {
		ret->shouldClear = true;
		sites = conj_(sites, (lisp_object*)ret);
		setVar(CLEAR_SITES, assoc(getVar(CLEAR_SITES), (lisp_object*)b, (lisp_object*)sites));
	}

	return (Expr*)ret;
}

// VectorExpr
typedef struct {	// VectorExpr
	EXPR_BASE
	const IVector *args;
} VectorExpr;

static const lisp_object* EvalVector(const Expr *self) {
	assert(self->type == VECTOREXPR_type);
	const IVector *args = ((VectorExpr*)self)->args;
	const IVector *ret = (IVector*) EmptyVector;
	for(size_t i = 0; i < ret->obj.fns->ICollectionFns->count((ICollection*)ret); i++) {
		const Expr *e = (Expr*) args->obj.fns->IVectorFns->nth(args, i, NULL);
		ret = ret->obj.fns->IVectorFns->cons(ret, e->Eval(e));
	}

	return (lisp_object*) ret;
}

static Expr* NewVectorExpr(const IVector *args) {
	VectorExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = VECTOREXPR_type;
	ret->Eval = EvalVector;

	ret->args = args;
	return (Expr*) ret;
}

// static Expr* parseVectorExpr(

// IParser
typedef const Expr* (*IParser)(Expr_Context context, const lisp_object *form);
typedef struct {	// Special
	const Symbol *sym;
	IParser parse;
} Special;
const Special specials[] = {
	{&_DefSymbol, NULL},
	{&_quoteSymbol, &parseConstant},
};
const size_t special_count = sizeof(specials)/sizeof(specials[0]);
IParser isSpecial(const lisp_object *sym) {
	if(sym == NULL)
		return NULL;
	if(sym->type != SYMBOL_type)
		return NULL;
	bool (*Equals)(const struct lisp_object_struct *x, const struct lisp_object_struct *y) = ((lisp_object*)sym)->Equals;
	for(size_t i =0; i<special_count; i++) {
		if(Equals((lisp_object*)sym, (lisp_object*)specials[i].sym))
			return specials[i].parse;
	}
	return NULL;
}



__attribute__((unused)) static void consumeWhitespace(FILE *input) {
	int ch = getc(input);
	while(isspace(ch))
		ch = getc(input);
	ungetc(ch, input);
}

static int registerConstant(const lisp_object *obj) {
	if(!isBound(CONSTANTS))
		return -1;
	const IMap *ids = (IMap*) deref(CONSTANT_IDS);
	const lisp_object *o = ids->obj.fns->IMapFns->entryAt(ids, obj);
	assert(o->type == INTEGER_type);
	if(o)
		return IntegerValue((Integer*)o);

	const IVector *v = (IVector*) deref(CONSTANTS);
	setVar(CONSTANTS, (lisp_object*)v->obj.fns->IVectorFns->cons(v, o));
	int i = v->obj.fns->ICollectionFns->count((ICollection*)v);
	setVar(CONSTANT_IDS, (lisp_object*)ids->obj.fns->IMapFns->assoc(ids, o, (lisp_object*)NewInteger(i)));
	return i;
}

static const Expr* registerKeyword(const Keyword *kw) {
	if(!isBound(KEYWORDS))
		return NewKeywordExpr(kw);
	const IMap *keywordsMap = (IMap*) deref(KEYWORDS);
	const lisp_object *id = keywordsMap->obj.fns->IMapFns->entryAt(keywordsMap, (lisp_object*)kw);
	if(id == NULL) {
		setVar(KEYWORDS, (lisp_object*)keywordsMap->obj.fns->IMapFns->assoc(keywordsMap, (lisp_object*)kw, (lisp_object*)NewInteger(registerConstant((lisp_object*)kw))));
	}
	return NewKeywordExpr(kw);
}

static int registerKeywordCallsite(const Keyword *k) {
	if(!isBound(KEYWORD_CALLSITES)) {
		// Throw New IllegalAccessError("KEYWORD_CALLSITES is not bound");
	}
	const IVector *keywordCallSites = (IVector*)deref(KEYWORD_CALLSITES);
	keywordCallSites = keywordCallSites->obj.fns->IVectorFns->cons(keywordCallSites, (lisp_object*)k);
	setVar(KEYWORD_CALLSITES, (lisp_object*)keywordCallSites);
	return keywordCallSites->obj.fns->ICollectionFns->count((ICollection*)keywordCallSites) - 1;
}

__attribute__((unused)) static bool namesStaticMember(const Symbol *sym) {
	return getNamespaceSymbol(sym) && namespaceFor(CurrentNS(), sym) == NULL;
}

static void registerVar(const Var *v) {
	if(!isBound(v))
		return;
	IMap *varsMap = (IMap*) deref(VARS);
	const lisp_object *id = varsMap->obj.fns->IMapFns->entryAt(varsMap, (lisp_object*)v);
	if(id) {
		setVar(VARS, (lisp_object*)varsMap->obj.fns->IMapFns->assoc(varsMap, (lisp_object*)v, (lisp_object*)NewInteger(registerConstant((lisp_object*)v))));
	}
}

static Var* LookupVar(const Symbol *sym, bool InternNew, bool RegisterMacro) {
	Var *ret = NULL;
	if(getNamespaceSymbol(sym)) {
		Namespace *ns = namespaceFor(CurrentNS(), sym);
		if(ns == NULL)
			return NULL;
		const Symbol *name = internSymbol1(getNameSymbol(sym));
		if(InternNew && ns == CurrentNS()) {
			ret = internNS(CurrentNS(), name);
		} else {
			ret = findInternedVar(ns, name);
		}
	}
	else if(((lisp_object*)sym)->Equals((lisp_object*)sym, (lisp_object*)namespaceSymbol)) {
		ret = NS_Var;
	}
	else if(((lisp_object*)sym)->Equals((lisp_object*)sym, (lisp_object*)inNamespaceSymbol)) {
		ret = IN_NS_Var;
	}
	else {
	}

	if(ret && (isMacroVar(ret) || RegisterMacro))
		registerVar(ret);
	return ret;
}

static Var* isMacro(const lisp_object *obj) {
	Var *ret = NULL;
	switch(obj->type) {
		case SYMBOL_type:
			ret = LookupVar((Symbol*)obj, false, false);
		case VAR_type:
			ret = (Var*)obj;
			break;
		default:
			return NULL;
	}
	if(ret && isMacroVar(ret)) {
		if(getNamespaceVar(ret) != CurrentNS() && !isPublic(ret)) {
			// throw new IllegalStateException("var: " + v + " is not public");
		}
		return ret;
	}
	return NULL;
}

static const lisp_object* macroExpand1(const lisp_object *x) {
	if(!isISeq(x))
		return x;
	const ISeq *form = (ISeq*)x;
	const lisp_object *op = form->obj.fns->ISeqFns->first(form);
	if(isSpecial(op))
		return x;
	const Var *v = isMacro(op);
	if(v) {
		const ISeq *args = form->obj.fns->ISeqFns->next(form);
		args = cons(getVar(LOCAL_ENV), (lisp_object*)args);
		args = cons(x, (lisp_object*)args);
		return ((lisp_object*)v)->fns->IFnFns->applyTo((IFn*)v, args);
	}
	// This appears to only be necesarry for the Java implementation.
	// if(op->type == SYMBOL_type) {
	// 	const Symbol *sym = (Symbol*) op;
	// 	if(namesStaticMember(sym)) {
	// 	}
	// 	// TODO macroExpand1
	// }
	return x;
}

static const lisp_object* macroExpand(const lisp_object *form) {
	const lisp_object *exp = macroExpand1(form);
	if(exp != form) 
		return macroExpand1(exp);
	return form;
}

static const Expr* AnalyzeSeq(Expr_Context context, const ISeq *form, char *name) {
	const lisp_object *me = macroExpand1((lisp_object*)form);
	if(me != (lisp_object*)form)
		return Analyze(context, me, name);

	const lisp_object *op = form->obj.fns->ISeqFns->first(form);
	if(op == NULL) {
		// Throw new IllegalArgumentException("Can't call nil, form: " + form);
		return NULL;
	}
	// const ISeq *next = form->obj.fns->ISeqFns->next(form);
	// IFn *inlined = isInline(op, next->obj.fns->ICollectionFns->count((ICollection*)next));	// TODO isInline
	// if(inlined != NULL)
	// 	return Analyze(context, preserveTag(form, inlined->obj.fns->IFnFns->applyTo(inlined, next)), name);	// TODO preserveTag
	if(((lisp_object*)FNSymbol)->Equals((lisp_object*)FNSymbol, op)) {
		// TODO return FnExpr.parse(context, form, name);
	}
	if(op->type == SYMBOL_type ) {
		IParser p = isSpecial(op);
		if(p) return p(context, (lisp_object*)form);
	}
	return parseInvokeExpr(context, form);
}

static const Expr* analyzeSymbol(const Symbol *sym) {
	const Symbol *tag = tagOf((lisp_object*)sym);
	if(getNamespaceSymbol(sym)) {
		LocalBinding *b = referenceLocal(sym);
		if(b)
			return NewLocalBindingExpr(b, tag);
	} else if(namespaceFor(CurrentNS(), sym) == NULL) {
		__attribute__((unused)) const Symbol *nsSym = internSymbol1(getNamespaceSymbol(sym));
		// TODO requires HostExpr.maybeClass and StaticFieldExpr
	}

	const lisp_object *o = resolveIn(CurrentNS(), sym, false);
	switch(o->type) {
		case VAR_type: {
			const Var *v = (Var*)o;
			if(isMacro((lisp_object*)v))
				return NULL;	// TODO throw Util.runtimeException("Can't take value of a macro: " + v);
			if(boolCast(get((lisp_object*)((lisp_object*)v)->meta, (lisp_object*)ConstKW, NULL)))
				return Analyze(EXPRESSION, (lisp_object*)listStar2((lisp_object*)quoteSymbol, getVar(v), NULL), NULL);
			registerVar(v);
			return NewVarExpr(v, tag);
		}
		// case CLASS_type:	// TODO
		// 	return NewConstantExpr(o);
		case SYMBOL_type:
			// return NewUnresolvedVarExpr((Symbol*)o);	// TODO
		default:
			return NULL;	// TODO throw Util.runtimeException("Unable to resolve symbol: " + sym + " in this context");
	}
}

static const Expr* Analyze(Expr_Context context, const lisp_object *form, __attribute__((unused)) char *name) {
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
		analyzeSymbol((Symbol*) form);
	}
	if(form->type == KEYWORD_type) {
		return registerKeyword((Keyword*)form);
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
		return AnalyzeSeq(context, (ISeq*)form, name);
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

static bool IsLiteralExpr(const Expr *e) {
	return e->type == KEYWORDEXPR_type || e->type == NUMBEREXPR_type || e->type == CONSTANTEXPR_type ||
			e->type == NILEXPR_type || e->type == BOOLEXPR_type || e->type == STRINGEXPR_type;
}

static const Symbol* tagOf(const lisp_object *o) {
	const IMap *meta = o->meta;
	const lisp_object *tag = meta->obj.fns->IMapFns->entryAt(meta, (lisp_object*)tagKW);

	if(tag) {
		switch(tag->type) {
			case SYMBOL_type:
				return (Symbol*) tag;
			case STRING_type:
				return internSymbol2(NULL, toString(tag));
			default:
				return NULL;
		}
	}
	return NULL;
}

static bool inTailCall(Expr_Context C) {
	return (C == RETURN) && deref(METHOD_RETURN_CONTEXT) && (deref(IN_CATCH_FINALLY) == NULL);
}

static const char* munge(const char *name) {
	// TODO munge
	return name;
}

__attribute__((unused)) static const char* demunge(const char *mungedName) {
	// TODO demunge
	return mungedName;
}

static const object_type* maybePrimitiveType(__attribute__((unused)) const Expr *e) {
	// TODO maybePrimitiveType
	return NULL;
}

static const lisp_object* resolveIn(Namespace *n, const Symbol *sym, bool allowPrivate) {
	if(getNamespaceSymbol(sym)) {
		const Namespace *ns = namespaceFor(n, sym);
		if(ns == NULL)
			return NULL;	// TODO throw Util.runtimeException("No such namespace: " + sym.ns);
		const Var *v = findInternedVar(ns, internSymbol1(getNameSymbol(sym)));
		if(v == NULL)
			return NULL;	// TODO throw Util.runtimeException("No such var: " + sym);
		if(getNamespaceVar(v) != CurrentNS() && !isPublic(v) && !allowPrivate)
			return NULL;	// TODO throw new IllegalStateException("var: " + sym + " is not public");
		return (lisp_object*) v;
	}

	const char *symName = getNameSymbol(sym);
	const char *dot = strchr(symName, '.');
	if((dot && symName != dot) || *symName == '[')
		return NULL;	// TODO return RT.classForName(sym.name);
	if(Equals((lisp_object*)sym, (lisp_object*)nsSymbol))
		return (lisp_object*) NS_Var;
	if(Equals((lisp_object*)sym, (lisp_object*)in_nsSymbol))
		return (lisp_object*) IN_NS_Var;
	if(Equals((lisp_object*)sym, getVar(COMPILE_STUB_SYM)))
		return getVar(COMPILE_STUB_CLASS);
	const lisp_object *o = getMapping(n, sym);
	if(o)
		return o;
	if(boolCast(deref(ALLOW_UNRESOLVED_VARS)))
		return (lisp_object*)sym;
	return NULL; // TODO throw Util.runtimeException("Unable to resolve symbol: " + sym + " in this context");
}


void initCompiler(void) {
	ALLOW_UNRESOLVED_VARS = setDynamic(internVar(LISP_ns, internSymbol1("*allow-unresolved-vars*"), (lisp_object*)False, true));
	CLEAR_PATH = setDynamic(createVar(NULL));
	CLEAR_ROOT = setDynamic(createVar(NULL));
	CLEAR_SITES = setDynamic(createVar(NULL));
	COMPILE_STUB_SYM = setDynamic(createVar(NULL));
	COMPILE_STUB_CLASS = setDynamic(createVar(NULL));
	CONSTANTS = setDynamic(createVar(NULL));
	CONSTANT_IDS = setDynamic(createVar(NULL));
	INSTANCE = internNS(findOrCreateNS(internSymbol1("lisp.core")), internSymbol1("instance?"));
	IN_CATCH_FINALLY = setDynamic(createVar(NULL));
	KEYWORD_CALLSITES = setDynamic(createVar(NULL));
	KEYWORDS = setDynamic(createVar(NULL));
	METHOD = setDynamic(createVar(NULL));
	METHOD_RETURN_CONTEXT = setDynamic(createVar(NULL));
	LOCAL_ENV = setDynamic(createVar(NULL));
	SOURCE = setDynamic(internVar(findOrCreateNS(internSymbol1("lisp.core")), internSymbol1("*source-path*"), (lisp_object*)NewString("NO_SOURCE_FILE"), true));
	VARS = setDynamic(createVar(NULL));
}

Namespace* CurrentNS(void) {
	const lisp_object *ret = deref(Current_ns);
	assert(ret->type == NAMESPACE_type);
	return (Namespace*)ret;
}

const lisp_object* Eval(const lisp_object *form) {
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
	for(const lisp_object *r = read(reader, false, '\0'); r->type != EOF_type; r = read(reader, false, '\0')) {
		printf("in compilerLoad loop.\n");
		printf("r = %p\n", (void*)r);
		printf("r->type = %d\n", r->type);
		printf("r->type = %s\n", object_type_string[r->type]);
		fflush(stdout);
		if(r->type == ERROR_type) {
			fprintf(stderr, "Error: %s\n", toString(r));
			break;
		}
		// TODO
		ret = Eval(r);
	}
	// TODO
	return ret;
}
