#include "Compiler.h"

#include <ctype.h>
#include <string.h>

#include "Bool.h"
#include "Error.h"
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
Var *NEXT_LOCAL_NUM = NULL;
Var *NO_RECUR = NULL;
Var *LOCAL_ENV = NULL;
Var *LOOP_LOCALS = NULL;
Var *SOURCE = NULL;
Var *SOURCE_PATH = NULL;
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
	LETEXPR_type,
	METAEXPR_type,
	MAPEXPR_type,
	KEYWORDEXPR_type,
	KWINVOKEEXPR_type,
	INVOKEEXPR_type,
	VAREXPR_type,
	LOCALBINDINGEXPR_type,
	OBJEXPR_type,
	VECTOREXPR_type,
	DEFEXPR_type,
	BODYEXPR_type,
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
static const Expr* Analyze(Expr_Context context, const lisp_object *form, const char *name);
static bool IsLiteralExpr(const Expr *e);
static const Expr* NewStringExpr(const String *form);
static const Expr* NewEmptyExpr(const ICollection *form);
static const Symbol* tagOf(const lisp_object *o);
static bool inTailCall(Expr_Context C);
static const char* munge(const char *name);
static const char* demunge(const char *mungedName);
static const object_type* maybePrimitiveType(const Expr *e);
static const lisp_object* resolveIn(Namespace *ns, const Symbol *sym, bool allowPrivate);
static Var* LookupVar(const Symbol *sym, bool InternNew, bool RegisterMacro);
static void registerVar(const Var *v);

// PathNode
typedef struct PathNode_struct {
	lisp_object obj;
	PATHTYPE type;
	const struct PathNode_struct *parent;
} PathNode;

static PathNode* NewPathNode(PATHTYPE type, const PathNode *parent) {
	PathNode *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = PATHNODE_type;
	ret->obj.size = sizeof(*ret);
	ret->obj.fns = &NullInterface;

	ret->type = type;
	ret->parent = parent;
	return ret;
}

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
	ObjExpr *objx;
	const IVector argLocals;
	size_t maxLocal;
	size_t line;
	size_t column;
	bool useThis;
	const IMap /* TODO ISet */ *localsUsedInCatchFinally;
	const IMap *methodMeta;
} ObjMethod;

const ObjMethod* NewObjMethod(ObjExpr *objx, ObjMethod *parent) {
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
	const IMap /* TODO ISet */ *usedConstants;
	int constantsID;
	int altCtorDrops;
	const IVector *keywordCallsites;
	const IVector *protocolCallsites;
	const IMap /* TODO ISet */ *varCallsites;
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
		exception e = {UnsupportedOperationException, "Can't type hint a local with a primitive initializer"};
		Raise(e);
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
			method->objx->closes = (IMap*)assoc((lisp_object*)method->objx->closes, (lisp_object*)b, (lisp_object*)b);
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

static size_t getAndIncLocalNum() {
	const Integer *I = (Integer*)deref(NEXT_LOCAL_NUM);
	assert(((lisp_object*)I)->type == INTEGER_type);
	const size_t i = IntegerValue(I);
	ObjMethod *m = (ObjMethod*)deref(METHOD);
	if(i > m->maxLocal)
		m->maxLocal = i;
	setVar(NEXT_LOCAL_NUM, (lisp_object*)NewInteger(i+1));
	return i;
}

static const LocalBinding* registerLocal(const Symbol *sym,  const Symbol *tag, const Expr *init, bool isArg) {
	size_t num = getAndIncLocalNum();
	const LocalBinding *b = NewLocalBinding(num, sym, tag, init, isArg, (PathNode*) getVar(CLEAR_ROOT));
	const IMap *localsMap = (IMap*)deref(LOCAL_ENV);
	setVar(LOCAL_ENV, assoc((lisp_object*)localsMap, (lisp_object*)b->sym, (lisp_object*)b));
	ObjMethod *method = (ObjMethod*)deref(METHOD);
	method->locals = (IMap*)assoc((lisp_object*)method->locals, (lisp_object*)b, (lisp_object*)b);
	method->indexLocals = (IMap*)assoc((lisp_object*)method->indexLocals, (lisp_object*)NewInteger(num), (lisp_object*)b);

	return b;
}

// BindingInit
typedef struct {
	lisp_object obj;
	const LocalBinding *binding;
	const Expr *Init;
} BindingInit;

static const BindingInit* NewBindingInit(const LocalBinding *binding, const Expr *Init) {
	BindingInit *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = BINDINGINIT_type;
	ret->obj.fns = &NullInterface;

	ret->binding = binding;
	ret->Init = Init;
	return ret;
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
		exception e = {RuntimeException, WriteString(AddString(AddInt(AddString(NewStringWriter(), "Wrong number of args ("), argcount), ") passed to quote"))};
		Raise(e);
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
			exception e = {IllegalArgumentException, "Duplicate constant keys in map."};
			Raise(e);
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
	TRY
		IFn *f = (IFn*) Invk->fexpr->Eval(Invk->fexpr);
		const IVector *args = (IVector*)EmptyVector;
		for(size_t i = 0; i < Invk->args->obj.fns->ICollectionFns->count((ICollection*)Invk->args); i++) {
			Expr *ith = (Expr*)Invk->args->obj.fns->IVectorFns->nth(Invk->args, i, NULL);
			args = args->obj.fns->IVectorFns->cons(args, ith->Eval(ith));
		}
		return f->obj.fns->IFnFns->applyTo(f, args->obj.fns->SeqableFns->seq((Seqable*)args));
	EXCEPT(CompilerExcp)
		ReRaise;
	EXCEPT(ANY)
		exception e = {CompilerException, _ctx.id->msg};
		Raise(e);
	ENDTRY
	__builtin_unreachable();
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
	exception e = {UnsupportedOperationException, "Can't eval locals"};
	Raise(e);
	__builtin_unreachable();
}

static Expr* NewLocalBindingExpr(LocalBinding *b, const Symbol *tag) {
	if(getPrimitiveType(b) && tag) {
		exception e = {UnsupportedOperationException, "Can't type hint a primitive local"};
		Raise(e);
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

static const Expr* parseVectorExpr(Expr_Context context, const IVector *form) {
	bool constant = true;
	const IVector *args = (IVector*)EmptyVector;
	for(size_t i = 0; i < count((lisp_object*)form); i++) {
		const Expr *v = Analyze(context == EVAL ? EVAL : EXPRESSION, form->obj.fns->IVectorFns->nth(form, i, NULL), NULL);
		args = args->obj.fns->IVectorFns->cons(args, (lisp_object*)v);
		if(!IsLiteralExpr(v))
			constant = false;
	}

	const Expr *ret = NewVectorExpr(args);
	if(((lisp_object*)form)->meta)
		return NewMetaExpr(ret, parseMapExpr(context == EVAL ? EVAL : EXPRESSION, ((lisp_object*)form)->meta));

	if(constant) {
		const IVector *rv = (IVector*)EmptyVector;
		for(size_t i = 0; i < count((lisp_object*)args); i++) {
			const Expr *ce = (Expr*) args->obj.fns->IVectorFns->nth(args, i, NULL);
			rv = rv->obj.fns->IVectorFns->cons(rv, ce->Eval(ce));
		}
		return NewConstantExpr((lisp_object*)rv);
	}

	return ret;
}

// DefExpr
typedef struct {	// DefExpr
	EXPR_BASE
	Var *v;
	const Expr *init;
	const Expr *meta;
	bool initProvided;
	bool isDynamic;
	bool shadowsCoreMapping;
	const char *source;
	size_t line;
	size_t column;
}DefExpr;

static const lisp_object* EvalDef(const Expr *self) {
	assert(self->type == DEFEXPR_type);
	const DefExpr *defx = (DefExpr*)self;
	TRY
		if(defx->initProvided) {
			bindRoot(defx->v, defx->init->Eval(defx->init));
			if(defx->meta) {
				const IMap *meta = (IMap*) defx->meta->Eval(defx->meta);
				setMeta(defx->v, meta);
			}
		}
		return (lisp_object*) setDynamic1(defx->v, defx->isDynamic);
	EXCEPT(CompilerExcp)
		ReRaise;
	EXCEPT(ANY)
		exception e = {CompilerException, _ctx.id->msg};
		Raise(e);
	ENDTRY
	__builtin_unreachable();
}

static Expr* NewDefExpr(const char *source, int line, int column, Var *v, const Expr *init, const Expr *meta, bool initProvided, bool isDynamic, bool shadowsCoreMapping) {
	DefExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = DEFEXPR_type;
	ret->Eval = EvalDef;

	ret->source = source;
	ret->line = line;
	ret->column = column;
	ret->v = v;
	ret->init = init;
	ret->meta = meta;
	ret->initProvided = initProvided;
	ret->isDynamic = isDynamic;
	ret->shadowsCoreMapping = shadowsCoreMapping;

	return (Expr*)ret;
}

static const Expr* parseDefExpr(Expr_Context context, const lisp_object *form) {
	const String *docstring = NULL;
	if(count(form) == 4 && third(form)->type == STRING_type) {
		docstring = (String*)third(form);
		form = (lisp_object*)listStar3(first(form), second(form), fourth(form), NULL);
	}

	if(count(form) > 3) {
		exception e = {RuntimeException, "Too many arguments to def"};
		Raise(e);
	} else if(count(form) < 2) {
		exception e = {RuntimeException, "Too few arguments to def"};
		Raise(e);
	} else if(second(form)->type != SYMBOL_type) {
		exception e = {RuntimeException, "First argument to def must be a Symbol"};
		Raise(e);
	}

	const Symbol *sym = (Symbol*) second(form);
	Var *v = LookupVar(sym, true, true);
	if(v == NULL) {
		exception e = {RuntimeException, "Can't refer to qualified var that doesn't exist"};
		Raise(e);
	}
	bool shadowsCoreMapping = false;
	if(!Equals((lisp_object*)getNamespaceVar(v), (lisp_object*)CurrentNS())) {
		if(getNameSymbol(sym)) {
		exception e = {RuntimeException, "Can't create defs outside of current ns"};
		Raise(e);
		}
		v = internNS(CurrentNS(), sym);
		shadowsCoreMapping = true;
		registerVar(v);
	}

	const IMap *mm = ((lisp_object*)sym)->meta;
	bool isDynamic = boolCast(get((lisp_object*)mm, (lisp_object*)DynamicKW, NULL));
	if(isDynamic)
		setDynamic(v);

	const lisp_object *arglists = get((lisp_object*)mm, (lisp_object*)arglistsKW, NULL);
	if(boolCast(arglists)) {
		const IMap* vm = ((lisp_object*)v)->meta;
		vm = (IMap*)assoc((lisp_object*)vm, (lisp_object*)arglistsKW, second(arglists));
		setMeta(v, vm);
	}

	const lisp_object *source_path =  getVar(SOURCE_PATH);
	if(source_path == NULL)
		source_path = (lisp_object*) NewString("No_Source_File");
	mm = (IMap*)assoc((lisp_object*)mm, (lisp_object*)FileKW, source_path);
	// mm = (IMap*)assoc((lisp_object*)mm, (lisp_object*)LineKW, getVar(LINE));		// TODO
	// mm = (IMap*)assoc((lisp_object*)mm, (lisp_object*)ColumnKW, getVar(COLUMN));		// TODO
	if(docstring)
		mm = (IMap*)assoc((lisp_object*)mm, (lisp_object*)DocKW, (lisp_object*)docstring);
	// mm = elideMeta(mm);	// TODO requires getCompilerOption
	const Expr *meta = (count((lisp_object*)mm) == 0) ? NULL : Analyze(context == EVAL ? EVAL : EXPRESSION, (lisp_object*)mm, NULL);

	return NewDefExpr(toString(deref(SOURCE)), 0 /* TODO linederef() */, 0 /* TODO columnderef() */, v,
			Analyze(context == EVAL ? EVAL : EXPRESSION, third(form), getNameSymbol(getSymbolVar(v))),
			meta, count(form) == 3, isDynamic, shadowsCoreMapping);
}

// BodyExpr
typedef struct {	// BodyExpr
	EXPR_BASE
	const IVector *exprs;
} BodyExpr;

static const lisp_object* EvalBody(const Expr *self) {
	assert(self->type == BODYEXPR_type);
	const BodyExpr *b = (BodyExpr*)self;
	const lisp_object *ret = NULL;
	for(const ISeq *s = b->exprs->obj.fns->SeqableFns->seq((Seqable*)b->exprs); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const Expr *e = (Expr*) s->obj.fns->ISeqFns->first(s);
		ret = e->Eval(e);
	}
	return ret;
}

static const Expr* NewBodyExpr(const IVector *exprs) {
	BodyExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = BODYEXPR_type;
	ret->Eval = EvalBody;

	ret->exprs = exprs;
	return (Expr*)ret;
}

static const Expr* parseBodyExpr(Expr_Context context, const lisp_object *form) {
	assert(isISeq(form));
	const ISeq *forms = (ISeq*)form;
	if(Equals((lisp_object*)DoSymbol, first((form))))
		forms = next((lisp_object*)forms);

	const IVector *exprs = (IVector*)EmptyVector;
	for(; forms != NULL; forms = forms->obj.fns->ISeqFns->next(forms)) {
		const Expr *e = Analyze(
				(context != EVAL && (context == STATEMENT || forms->obj.fns->ISeqFns->next(forms))) ? STATEMENT : context,
				forms->obj.fns->ISeqFns->first(forms),
				NULL);
		exprs = exprs->obj.fns->IVectorFns->cons(exprs, (lisp_object*)e);
	}

	if(count((lisp_object*)exprs) == 0)
		exprs = exprs->obj.fns->IVectorFns->cons(exprs, (lisp_object*)NilExpr);
	return NewBodyExpr(exprs);
}

// LetExpr
typedef struct {	// LetExpr
	EXPR_BASE
	const IVector *bindingInits;
	const Expr *body;
	bool isLoop;
} LetExpr;

static const lisp_object* EvalLet(__attribute__((unused)) const Expr *self) {
	exception e = {UnsupportedOperationException, "Can't eval let/loop"};
	Raise(e);
	__builtin_unreachable();
}

static Expr* NewLetExpr(const IVector *bindingInits, const Expr *body, bool isLoop) {
	LetExpr *ret = GC_MALLOC(sizeof(*ret));
	ret->obj.type = EXPR_type;
	ret->obj.fns = &NullInterface;
	ret->type = LETEXPR_type;
	ret->Eval = EvalLet;

	ret->bindingInits = bindingInits;
	ret->body = body;
	ret->isLoop = isLoop;
	return (Expr*) ret;
}

static const Expr* parseLetExpr(Expr_Context context, const lisp_object *form) {
	bool isLoop = Equals(first(form), (lisp_object*)LoopSymbol);
	if(!isIVector(second(form))) {
		exception e = {IllegalArgumentException, "Bad binding form, expected vector"};
		Raise(e);
	}

	const IVector *bindings = (IVector*) second(form);
	if(bindings->obj.fns->ICollectionFns->count((ICollection*)bindings) & 1) {
		exception e = {IllegalArgumentException, "Bad binding form, expected matched symbol expression pairs"};
		Raise(e);
	}

	const ISeq *body = next((lisp_object*)next(form));

	if(context == EVAL || (context == EXPRESSION && isLoop))
		Analyze(context, (lisp_object*)listStar1((lisp_object*)listStar2((lisp_object*)FnOnceSymbol, (lisp_object*)EmptyVector, (ISeq*)form), NULL), NULL);
	ObjMethod *method = (ObjMethod*)deref(METHOD);
	const IMap *backupMethodLocals = method->locals;
	const IMap *backupMethodIndexLocals = method->indexLocals;
	const IVector *recurMismatches = (IVector*)EmptyVector;

	for(size_t i = 0; i < bindings->obj.fns->ICollectionFns->count((ICollection*)bindings)/2; i++)
		recurMismatches = recurMismatches->obj.fns->IVectorFns->cons(recurMismatches, (lisp_object*)False);

	while(true) {
		const lisp_object *mapArgs[] = {(lisp_object*)LOCAL_ENV, deref(LOCAL_ENV), (lisp_object*)NEXT_LOCAL_NUM, deref(NEXT_LOCAL_NUM)};
		size_t mapArgc = sizeof(mapArgs)/sizeof(mapArgs[0]);
		const IMap *dynamicBindings = (IMap*)CreateHashMap(mapArgc, mapArgs);

		method->locals = backupMethodLocals;
		method->indexLocals = backupMethodIndexLocals;
		const PathNode *loopRoot = NewPathNode(PATH, (PathNode*)getVar(CLEAR_PATH));
		const PathNode *clearRoot = NewPathNode(PATH, loopRoot);
		const PathNode *clearPath = NewPathNode(PATH, loopRoot);

		if(isLoop)
			dynamicBindings = dynamicBindings->obj.fns->IMapFns->assoc(dynamicBindings, (lisp_object*)LOOP_LOCALS, NULL);
		TRY
			pushThreadBindings(dynamicBindings);
			const IVector *bindingInits = (IVector*)EmptyVector;
			const IVector *loopLocals = (IVector*)EmptyVector;
			for(size_t i = 0; i < bindings->obj.fns->ICollectionFns->count((ICollection*)bindings); i+=2) {
				const lisp_object *obj = bindings->obj.fns->IVectorFns->nth(bindings, i, NULL);
				if(obj->type != SYMBOL_type) {
					exception e = {IllegalArgumentException, WriteString(AddString(AddString(NewStringWriter(),
									"Bad binding form, expected symbol, got: "), toString(obj)))};
					Raise(e);
				}
				const Symbol *sym = (Symbol*) obj;
				if(getNamespaceSymbol(sym)) {
					exception e = {RuntimeException, WriteString(AddString(AddString(NewStringWriter(),
									"Can't let qualified name: "), toString(obj)))};
					Raise(e);
				}
				const Expr *init = Analyze(EXPRESSION, bindings->obj.fns->IVectorFns->nth(bindings, i+1, NULL), getNameSymbol(sym));
				if(isLoop) {
					if(boolCast(recurMismatches->obj.fns->IVectorFns->nth(recurMismatches, 1/2, NULL))) {
						// init = StaticMethodExpr(...)		// TODO
					} else if(*maybePrimitiveType(init) == INTEGER_type) {
						// init = StaticMethodExpr(...)		// TODO
					} else if(*maybePrimitiveType(init) == FLOAT_type) {
						// init = StaticMethodExpr(...)		// TODO
					}
				}

				TRY
					if(isLoop) {
						const lisp_object* mapArgs[] = {
							(lisp_object*)CLEAR_PATH, (lisp_object*)clearPath,
							(lisp_object*)CLEAR_ROOT, (lisp_object*)clearRoot,
							(lisp_object*)NO_RECUR, NULL
						};
						size_t mapArgc = sizeof(mapArgs)/sizeof(mapArgs[0]);
						pushThreadBindings((IMap*)CreateHashMap(mapArgc, mapArgs));
						const LocalBinding *lb = registerLocal(sym, tagOf((lisp_object*)sym), init, false);
						const BindingInit *bi = NewBindingInit(lb, init);
						bindingInits = bindingInits->obj.fns->IVectorFns->cons(bindingInits, (lisp_object*)bi);
						if(isLoop)
							loopLocals = loopLocals->obj.fns->IVectorFns->cons(loopLocals, (lisp_object*)bi);
					}
				FINALLY
					if(isLoop)
						popThreadBindings();
				ENDTRY
			}
			if(isLoop)
				setVar(LOOP_LOCALS, (lisp_object*)loopLocals);

			const Expr *bodyExpr = NULL;
			bool moreMismatches = false;
			TRY
				if(isLoop) {
					const lisp_object *mapArgs[] = {
						(lisp_object*)CLEAR_PATH, (lisp_object*)clearPath,
						(lisp_object*)CLEAR_ROOT, (lisp_object*)clearRoot,
						(lisp_object*)NO_RECUR, NULL,
						(lisp_object*)METHOD_RETURN_CONTEXT, (context == RETURN ? deref(METHOD_RETURN_CONTEXT) : NULL)
					};
					size_t mapArgc = sizeof(mapArgs)/sizeof(mapArgs[0]);
					pushThreadBindings((IMap*)CreateHashMap(mapArgc, mapArgs));
					bodyExpr = parseBodyExpr((isLoop ? RETURN: context), (lisp_object*)body);
				}
			FINALLY
				if(isLoop) {
					popThreadBindings();
					for(size_t i = 0; i < count((lisp_object*)loopLocals); i++) {
						const LocalBinding *lb = (LocalBinding*) loopLocals->obj.fns->IVectorFns->nth(loopLocals, i, NULL);
						if(lb->recurMismatch) {
							recurMismatches = recurMismatches->obj.fns->IVectorFns->assoc(recurMismatches, (lisp_object*)NewInteger(i), (lisp_object*)True);
							moreMismatches = true;
						}
					}
				}
			ENDTRY
			if(!moreMismatches)
				return NewLetExpr(bindingInits, bodyExpr, isLoop);
		FINALLY
			popThreadBindings();
		ENDTRY
	}
}

// IParser
typedef const Expr* (*IParser)(Expr_Context context, const lisp_object *form);
typedef struct {	// Special
	const Symbol *sym;
	IParser parse;
} Special;
const Special specials[] = {
	{&_DefSymbol, &parseDefExpr},
	// Loop			// TODO
	// Recur		// TODO
	// if			// TODO
	// case			// TODO
	{&_LetSymbol, &parseLetExpr},
	// LetFn		// TODO
	{&_DoSymbol, &parseBodyExpr},
	{&_quoteSymbol, &parseConstant},
	// TheVar		// TODO
	// Import		// TODO
	// Dot			// TODO
	// Assign		// TODO
	// DefType		// TODO
	// Reify		// TODO
	// Try			// TODO
	// Throw		// TODO
	// MonitorEnter	// TODO
	// MonitorExit	// TODO
	// Catch		// TODO
	// Finally		// TODO
	// New			// TODO
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
		exception e = {IllegalAccessError, "KEYWORD_CALLSITES is not bound"};
		Raise(e);
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
	const lisp_object *id = get((lisp_object*)varsMap, (lisp_object*)v, NULL);
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
			break;
		case VAR_type:
			ret = (Var*)obj;
			break;
		default:
			return NULL;
	}
	if(ret && isMacroVar(ret)) {
		if(getNamespaceVar(ret) != CurrentNS() && !isPublic(ret)) {
			exception e = {IllegalStateException, WriteString(AddString(AddString(AddString(NewStringWriter(), "var: "), toString((lisp_object*)ret)), " is not public"))};
			Raise(e);
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
	if(isSpecial(op)) {
		return x;
	}
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
	// 	macroExpand1
	// }
	return x;
}

static const lisp_object* macroExpand(const lisp_object *form) {
	const lisp_object *exp = macroExpand1(form);
	if(exp != form) 
		return macroExpand1(exp);
	return form;
}

static const Expr* AnalyzeSeq(Expr_Context context, const ISeq *form, const char *name) {
	const lisp_object *me = macroExpand1((lisp_object*)form);
	if(me != (lisp_object*)form)
		return Analyze(context, me, name);

	const lisp_object *op = form->obj.fns->ISeqFns->first(form);
	if(op == NULL) {
		exception e = {IllegalArgumentException, WriteString(AddString(AddString(NewStringWriter(), "Can't call nil, form: "), toString((lisp_object*)form)))};
		Raise(e);
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
	printf("In AnanlyzeSymbol.\n");
	printf("sym = %p\n", (void*)sym);
	if(sym)
		printf("sym = %s\n", toString((lisp_object*)sym));
	const char *ns = getNamespaceSymbol(sym);
	printf("Got ns.\n");
	printf("ns = %p\n", ns);
	if(ns)
		printf("sym.ns = %s\n", ns);
	fflush(stdout);
	const Symbol *tag = tagOf((lisp_object*)sym);
	printf("tag = %p\n", (void*)tag);
	if(tag)
		printf("tag = %s\n", toString((lisp_object*)tag));
	fflush(stdout);
	if(getNamespaceSymbol(sym) == NULL) {
		LocalBinding *b = referenceLocal(sym);
		printf("b = %p\n", (void*) b);
		fflush(stdout);
		if(b)
			return NewLocalBindingExpr(b, tag);
	} else if(namespaceFor(CurrentNS(), sym) == NULL) {
		__attribute__((unused)) const Symbol *nsSym = internSymbol1(getNamespaceSymbol(sym));
		// TODO requires HostExpr.maybeClass and StaticFieldExpr
	}
	printf("after if/else clause.\n");
	fflush(stdout);

	printf("CurrentNS = %s\n", toString((lisp_object*)CurrentNS()));
	fflush(stdout);

	const lisp_object *o = resolveIn(CurrentNS(), sym, false);
	printf("Got o.\n");
	printf("o = %p\n", (void*)o);
	if(o)
		printf("o = %s\n", toString(o));
	fflush(stdout);
	switch(o->type) {
		case VAR_type: {
			const Var *v = (Var*)o;
			if(isMacro((lisp_object*)v)) {
				exception e = {RuntimeException, WriteString(AddString(AddString(NewStringWriter(), "Can't take value of a macro: "), toString(o)))};
				Raise(e);
			}
			if(boolCast(get((lisp_object*)((lisp_object*)v)->meta, (lisp_object*)ConstKW, NULL)))
				return Analyze(EXPRESSION, (lisp_object*)listStar2((lisp_object*)quoteSymbol, getVar(v), NULL), NULL);
			registerVar(v);
			return NewVarExpr(v, tag);
		}
		// case CLASS_type:	// TODO
		// 	return NewConstantExpr(o);
		case SYMBOL_type:
			// return NewUnresolvedVarExpr((Symbol*)o);	// TODO
		default: {
			exception e = {RuntimeException, WriteString(AddString(AddString(AddString(NewStringWriter(), "Unable to resolve symbol: "),
							toString((lisp_object*)sym)), " in this context"))};
			Raise(e);
		}
	}
	__builtin_unreachable();
}

static const Expr* Analyze(Expr_Context context, const lisp_object *form, const char *name) {
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
		return parseVectorExpr(context, (IVector*)form);
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
	const lisp_object *tag = get((lisp_object*)o->meta, (lisp_object*)tagKW, NULL);

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
	return NULL;	// TODO maybePrimitiveType
}

static const lisp_object* resolveIn(Namespace *n, const Symbol *sym, bool allowPrivate) {
	printf("In resolveIn\n");
	printf("n = %s\n", toStringMapping(n));
	if(getNamespaceSymbol(sym)) {
		const Namespace *ns = namespaceFor(n, sym);
		if(ns == NULL) {
			exception e = {RuntimeException, WriteString(AddString(AddString(NewStringWriter(), "No such namespace: "), toString((lisp_object*)getNamespaceSymbol(sym))))};
			Raise(e);
		}
		const Var *v = findInternedVar(ns, internSymbol1(getNameSymbol(sym)));
		if(v == NULL) {
			exception e = {RuntimeException, WriteString(AddString(AddString(NewStringWriter(), "No such var: "), toString((lisp_object*)sym)))};
			Raise(e);
		}
		if(getNamespaceVar(v) != CurrentNS() && !isPublic(v) && !allowPrivate) {
			exception e = {RuntimeException, WriteString(AddString(AddString(AddString(NewStringWriter(), "var: "), toString((lisp_object*)sym)), " is not public"))};
			Raise(e);
		}
		return (lisp_object*) v;
	}

	const char *symName = getNameSymbol(sym);
	const char *dot = strchr(symName, '.');
	if((dot && symName != dot) || *symName == '[')
		return NULL;	// TODO return RT.classForName(sym.name);
	if(Equals((lisp_object*)sym, (lisp_object*)nsSymbol)) {
		return (lisp_object*) NS_Var;
	}
	if(Equals((lisp_object*)sym, (lisp_object*)in_nsSymbol))
		return (lisp_object*) IN_NS_Var;
	if(Equals((lisp_object*)sym, getVar(COMPILE_STUB_SYM)))
		return getVar(COMPILE_STUB_CLASS);
	const lisp_object *o = getMapping(n, sym);
	if(o)
		return o;
	if(boolCast(deref(ALLOW_UNRESOLVED_VARS)))
		return (lisp_object*)sym;
	exception e = {RuntimeException, WriteString(AddString(AddString(AddString(NewStringWriter(), "Unable to resolve symbol: "), toString((lisp_object*)sym)), " in this context"))};
	Raise(e);
	__builtin_unreachable();
}


void initCompiler(void) {
	const lisp_object *mapArgs[] = {(lisp_object*)internKeyword2(NULL, "once"), (lisp_object*)True};
	FnOnceSymbol = (Symbol*)withMeta((lisp_object*)FnOnceSymbol, (IMap*)CreateHashMap(2, mapArgs));

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
	NEXT_LOCAL_NUM = setDynamic(createVar((lisp_object*)NewInteger(0)));
	NO_RECUR = setDynamic(createVar(NULL));
	LOCAL_ENV = setDynamic(createVar(NULL));
	LOOP_LOCALS = setDynamic(createVar(NULL));
	SOURCE = setDynamic(internVar(findOrCreateNS(internSymbol1("lisp.core")), internSymbol1("*source-path*"), (lisp_object*)NewString("NO_SOURCE_FILE"), true));
	SOURCE_PATH = setDynamic(internVar(findOrCreateNS(internSymbol1("lisp.core")), internSymbol1("*file*"), (lisp_object*)NewString("NO_SOURCE_PATH"), true));
	VARS = setDynamic(createVar(NULL));
}

Namespace* CurrentNS(void) {
	const lisp_object *ret = deref(Current_ns);
	assert(ret->type == NAMESPACE_type);
	return (Namespace*)ret;
}

const lisp_object* Eval(const lisp_object *form) {
	printf("form = %s\n", toString(form));
	fflush(stdout);
	form = macroExpand(form);
	printf("form = %s\n", toString(form));
	printf("form->type = %s\n", object_type_string[form->type]);
	fflush(stdout);
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
	const lisp_object *mapArgs[] = {
		(lisp_object*)SOURCE_PATH, (lisp_object*)NewString(path),
		(lisp_object*)SOURCE, (lisp_object*)NewString(name),
		(lisp_object*)METHOD, NULL,
		(lisp_object*)LOCAL_ENV, NULL,
		// LOOP_LOCALS, NULL,	// TODO
		(lisp_object*)NEXT_LOCAL_NUM, (lisp_object*)NewInteger(0),
		// READEVAL, True,	// TODO
		(lisp_object*)Current_ns, deref(Current_ns),
		// LINE_BEFORE	// TODO
		// COLUMN_BEFORE	// TODO
		// LINE_AFTER	// TODO
		// COLUMN_AFTER	// TODO
		// UNCHECKED_MATH, deref(UNCHECKED_MATH),	// TODO
		// WARN_ON_REFLECTION, deref(WARN_ON_REFLECTION),	// TODO
		// DATA_READERS, deref(DATA_READERS),	// TODO
	};
	size_t mapArgc = sizeof(mapArgs)/sizeof(mapArgs[0]);
	pushThreadBindings((IMap*)CreateHashMap(mapArgc, mapArgs));
	TRY
		for(const lisp_object *r = read(reader, false, '\0'); r->type != EOF_type; r = read(reader, false, '\0')) {
			if(r->type == ERROR_type) {
				break;
			}
			// set(LINE_AFTER, ...);	// TODO
			// set(COLUMN_AFTER, ...);	// TODO
			ret = Eval(r);
			// set(LINE_BEFORE, ...);	// TODO
			// set(COLUMN_BEFORE, ...);	// TODO
		}
	EXCEPT(CompilerExcp)
		ReRaise;
	EXCEPT(ANY)
		exception e = {CompilerException, _ctx.id->msg};
	Raise(e);
	FINALLY
		popThreadBindings();
	ENDTRY
		return ret;
}
