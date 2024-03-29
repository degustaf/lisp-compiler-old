#include "RunTime.h"

#include <string.h>
#include <stdio.h>	// For Debugging.

#include "AFn.h"
#include "Compiler.h"
#include "Error.h"
#include "gc.h"
#include "Keyword.h"
#include "List.h"
#include "LineNumberReader.h"
#include "lisp_pthread.h"
#include "Map.h"
#include "Reader.h"
#include "Strings.h"
#include "StringWriter.h"
#include "Symbol.h"
#include "Vector.h"

Namespace *LISP_ns = NULL;
Var *OUT = NULL;
Var *Current_ns = NULL;
Var *AGENT = NULL;
Var *NS_Var = NULL;
Var *IN_NS_Var = NULL;

static void load(const char *scriptbase, bool failIfNotFound);
static void loadResourceScript(char *name, bool failIfNotFound);

static const lisp_object *invokeBootNS(const IFn *self, const lisp_object *__form, const lisp_object *__env, const lisp_object *arg1);
const IFn_vtable bootNS_IFn_vtable = {
	invoke0AFn,		// invoke0
	invoke1AFn,		// invoke1
	invoke2AFn,		// invoke2
	invokeBootNS,	// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
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
const IFn bootNS = {{IFN_type, sizeof(IFn), NULL, NULL, (IMap*)&_EmptyHashMap, &bootNS_interfaces}};

static const lisp_object *invokeInNS(const IFn *self, const lisp_object *arg1);
const IFn_vtable InNS_IFn_vtable = {
	invoke0AFn,	// invoke0
	invokeInNS,	// invoke1
	invoke2AFn,	// invoke2
	invoke3AFn,	// invoke3
	invoke4AFn,	// invoke4
	invoke5AFn,	// invoke5
	applyToAFn,	// applyTo
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
const IFn InNS = {{IFN_type, sizeof(IFn), NULL, NULL, (IMap*)&_EmptyHashMap, &InNS_interfaces}};

static const lisp_object *invokeLoadFile(const IFn *self, const lisp_object *arg1);
const IFn_vtable LoadFile_IFn_vtable = {
	invoke0AFn,		// invoke0
	invokeLoadFile,	// invoke1
	invoke2AFn,		// invoke2
	invoke3AFn,		// invoke3
	invoke4AFn,		// invoke4
	invoke5AFn,		// invoke5
	applyToAFn,		// applyTo
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
const IFn LoadFile = {{IFN_type, sizeof(IFn), NULL, NULL, (IMap*)&_EmptyHashMap, &LoadFile_interfaces}};

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
	NS_Var = internVar(LISP_ns, namespaceSymbol, (lisp_object*)&bootNS, true);
	setMacro(NS_Var);

	const Keyword *arglistkw = internKeyword2(NULL, "arglists");
	const Symbol *nameSym = internSymbol1("name");
	args[1] = (lisp_object*)NewString("Sets *ns* to the namespace named by the symbol, creating it if needed.");
	args[2] = (lisp_object*)arglistkw;
	const Vector *vec = CreateVector(1, (const lisp_object**)&nameSym);
	args[3] = (lisp_object*) CreateList(1, (const lisp_object**)&vec);
	IN_NS_Var = internVar(LISP_ns, inNamespaceSymbol, (lisp_object*)&InNS, true);
	setMeta(IN_NS_Var, (IMap*)CreateHashMap(4, args));

	args[1] = (lisp_object*)NewString("Sequentially read and evaluate the set of forms contained in the file.");
	Var *v = internVar(LISP_ns, loadFileSymbol, (lisp_object*)&LoadFile, true);
	setMeta(v, (IMap*)CreateHashMap(4, args));
	initCompiler();

	printf("About to load.\n");
	fflush(stdout);
    init_reader();
	load("lisp/core", true);
}

static int id = 1;
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;
int nextID(void) {
	pthread_mutex_lock(&id_mutex);
	id++;
	pthread_mutex_unlock(&id_mutex);
	return id;
}

static const lisp_object *invokeBootNS(const IFn *self, __attribute__((unused)) const lisp_object *__form, __attribute__((unused)) const lisp_object *__env, const lisp_object *arg1) {
	printf("In invokeBootNS.\n");
	fflush(stdout);
	return invokeInNS(self, arg1);
}

static const lisp_object *invokeInNS(__attribute__((unused)) const IFn *self, const lisp_object *arg1) {
	printf("In invokeInNS.\n");
	fflush(stdout);
	assert(arg1->type == SYMBOL_type);
	const Symbol *nsName = (Symbol*)arg1;
	Namespace *ns = findOrCreateNS(nsName);
	setVar(Current_ns, (const lisp_object*)ns);
	return (const lisp_object*) ns;
}

static const lisp_object *invokeLoadFile(__attribute__((unused)) const IFn *self, __attribute__((unused)) const lisp_object *arg1) {
	printf("In invokeLoadFile.\n");
	fflush(stdout);
	return NULL;	// TODO requires compiler.loadFile
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
		printf("About to Enter loadResourceScript.\n");
		fflush(stdout);
		loadResourceScript(lispFile, true);
	} else if(!loaded && failIfNotFound) {
		StringWriter *sw = NewStringWriter();
		AddString(sw, "Could not locate ");
		AddString(sw, scriptbase);
		AddString(sw, " on classpath.");
		exception e = {FileNotFoundException, WriteString(sw)};
		Raise(e);
	}
}

static void loadResourceScript(char *name, bool failIfNotFound) {
	LineNumberReader *in = NewLineNumberReader(name);
	char *file = name;
	for(char *ptr = strchr(file, '/'); ptr != NULL; ptr = strchr(file, '/'))
		file = ptr + 1;
	if(in) {
		TRY
			printf("About to enter compilerLoad.\n");
			fflush(stdout);
			compilerLoad(in, name, file);
		FINALLY
			closeLineNumberReader(in);
		ENDTRY
	} else if(failIfNotFound) {
		StringWriter *sw = NewStringWriter();
		AddString(sw, "Could not locate Clojure resource on classpath: ");
		AddString(sw, name);
		exception e = {FileNotFoundException, WriteString(sw)};
		Raise(e);
	}
}
