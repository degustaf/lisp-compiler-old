#include "RunTime.h"

#include <string.h>

#include "AFn.h"
#include "Compiler.h"
#include "gc.h"
#include "Keyword.h"
#include "List.h"
#include "Map.h"
#include "Strings.h"
#include "Symbol.h"
#include "Vector.h"

Namespace *LISP_ns = NULL;
Var *OUT = NULL;
Var *Current_ns = NULL;
Var *AGENT = NULL;

static void load(const char *scriptbase, bool failIfNotFound);
static void loadResourceScript(char *name, bool failIfNotFound);

static const lisp_object *invokeBootNS(const IFn *self, const lisp_object *__form, const lisp_object *__env, const lisp_object *arg1);
IFn_vtable bootNS_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1AFn,		// invoke1
	invoke2AFn,		// invoke2
	invokeBootNS,	// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	NULL,			// applyTo
};
interfaces bootNS_interfaces = {
	NULL,				// SeqableFns
	NULL,				// ReversibleFns
	NULL,				// ICollectionFns
	NULL,				// IStackFns
	NULL,				// ISeqFns
	&bootNS_IFn_vtable,	// IFnFns
	NULL,				// IVectorFns
	NULL,				// IMapFns
};
const IFn bootNS = {{IFN_type, NULL, NULL, NULL, (IMap*)&_EmptyHashMap, &bootNS_interfaces}};

static const lisp_object *invokeInNS(const IFn *self, const lisp_object *arg1);
IFn_vtable InNS_IFn_vtable = {
	invoke0AFn,	// invoke0
	invokeInNS,	// invoke1
	invoke2AFn,	// invoke2
	invoke3AFn,	// invoke3
	invoke4AFn,	// invoke4
	invoke5AFn,	// invoke5
	NULL,		// applyTo
};
interfaces InNS_interfaces = {
	NULL,				// SeqableFns
	NULL,				// ReversibleFns
	NULL,				// ICollectionFns
	NULL,				// IStackFns
	NULL,				// ISeqFns
	&bootNS_IFn_vtable,	// IFnFns
	NULL,				// IVectorFns
	NULL,				// IMapFns
};
const IFn InNS = {{IFN_type, NULL, NULL, NULL, (IMap*)&_EmptyHashMap, &InNS_interfaces}};

static const lisp_object *invokeLoadFile(const IFn *self, const lisp_object *arg1);
IFn_vtable LoadFile_IFn_vtable = {
	invoke0AFn,		// invoke0
	invokeLoadFile,	// invoke1
	invoke2AFn,		// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	NULL,			// applyTo
};
interfaces LoadFile_interfaces = {
	NULL,					// SeqableFns
	NULL,					// ReversibleFns
	NULL,					// ICollectionFns
	NULL,					// IStackFns
	NULL,					// ISeqFns
	&LoadFile_IFn_vtable,	// IFnFns
	NULL,					// IVectorFns
	NULL,					// IMapFns
};
const IFn LoadFile = {{IFN_type, NULL, NULL, NULL, (IMap*)&_EmptyHashMap, &LoadFile_interfaces}};

const Var* RTVar(const char *ns, const char *name) {
	return internVar(findOrCreateNS(internSymbol2(NULL, ns)), internSymbol2(NULL, name), NULL, true);
}

void initRT(void) {
	const lisp_object *args[4] = {(lisp_object*)DocKW, NULL, NULL, NULL};
	LISP_ns = findOrCreateNS(internSymbol1("lisp.core"));
	OUT = internVar(LISP_ns, internSymbol1("*out*"), NULL, true);	// TODO convert NULL to appropriate object.
	setTag(OUT, internSymbol1("ioWriter"));
	Current_ns = setDynamic(internVar(LISP_ns, internSymbol1("*ns*"), (lisp_object*)LISP_ns, true));
	setTag(Current_ns, internSymbol1("lisp.Namespace"));

	args[1] = (lisp_object*)NewString("The agent currently running an action on this thread, else nil");
	AGENT = setDynamic(internVar(LISP_ns, internSymbol1("*agent*"), NULL, true));
	setMeta(AGENT, (IMap*)CreateHashMap(2, args));

	// MATH_CONTEXT			// TODO
	Var *v = internVar(LISP_ns, namespaceSymbol, (lisp_object*)&bootNS, true);
	setMacro(v);

	const Keyword *arglistkw = internKeyword2(NULL, "arglists");
	const Symbol *nameSym = internSymbol1("name");
	args[1] = (lisp_object*)NewString("Sets *ns* to the namespace named by the symbol, creating it if needed.");
	args[2] = (lisp_object*)arglistkw;
	const Vector *vec = CreateVector(1, (const lisp_object**)&nameSym);
	args[3] = (lisp_object*) CreateList(1, (const lisp_object**)&vec);
	v = internVar(LISP_ns, inNamespaceSymbol, (lisp_object*)&InNS, true);
	setMeta(v, (IMap*)CreateHashMap(4, args));

	args[1] = (lisp_object*)NewString("Sequentially read and evaluate the set of forms contained in the file.");
	v = internVar(LISP_ns, loadFileSymbol, (lisp_object*)&LoadFile, true);
	setMeta(v, (IMap*)CreateHashMap(4, args));

	load("lisp/core", true);
}

static const lisp_object *invokeBootNS(const IFn *self, __attribute__((unused)) const lisp_object *__form, __attribute__((unused)) const lisp_object *__env, const lisp_object *arg1) {
	return invokeInNS(self, arg1);
}

static const lisp_object *invokeInNS(__attribute__((unused)) const IFn *self, const lisp_object *arg1) {
	assert(arg1->type == SYMBOL_type);
	const Symbol *nsName = (Symbol*)arg1;
	Namespace *ns = findOrCreateNS(nsName);
	setVar(Current_ns, (const lisp_object*)ns);
	return (const lisp_object*) ns;
}

static const lisp_object *invokeLoadFile(__attribute__((unused)) const IFn *self, __attribute__((unused)) const lisp_object *arg1) {
	// TODO requires compiler.loadFile
	return NULL;
}

static void load(const char *scriptbase, bool failIfNotFound) {
	// char *objectFile = NULL;
	size_t len = strlen(scriptbase);
	char *lispFile = GC_MALLOC_ATOMIC(len + 5);
	strncpy(lispFile, scriptbase, len + 1);
	strcat(lispFile, ".lsp");
	bool loaded = false;
	// dlopen objectFile
	if(!loaded /* && ... */) {
		loadResourceScript(lispFile, true);
	} else if(!loaded && failIfNotFound) {
		// TODO Throw Error.
	}
}

static void loadResourceScript(char *name, bool failIfNotFound) {
	FILE *in = fopen(name, "r");
	char *file = name;
	for(char *ptr = strchr(file, '/'); ptr != NULL; ptr = strchr(file, '/'))
		file = ptr;
	if(in) {
		// try
		compilerLoad(in, name, file);
		// finally
		fclose(in);
	} else if(failIfNotFound) {
		// TODO throw Error
	}
}
