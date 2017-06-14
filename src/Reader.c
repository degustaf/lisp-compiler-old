#include "Reader.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#include "Bool.h"
#include "Compiler.h"
#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.
#include "gc.h"
#include "Keyword.h"
#include "List.h"
#include "Map.h"
#include "Namespace.h"
#include "Numbers.h"
#include "Strings.h"
#include "Symbol.h"
#include "Util.h"
#include "Vector.h"

const lisp_object DONE_lisp_object = {DONE_type, sizeof(lisp_object), NULL, NULL, NULL, NULL, NULL};
const lisp_object NOOP_lisp_object = {NOOP_type, sizeof(lisp_object), NULL, NULL, NULL, NULL, NULL};
const lisp_object EOF_lisp_object  = {EOF_type, sizeof(lisp_object), NULL, NULL, NULL, NULL, NULL};
regex_t symbol_regex;

typedef const lisp_object* (*MacroFn)(FILE*, char /* *lisp_object opts, *lisp_object pendingForms */);
static MacroFn macros[128];

// Static Function Declarations.
static MacroFn get_macro(int ch);
static bool isMacro(int ch);
static bool isTerminatingMacro(int ch);
static lisp_object *ReadNumber(FILE *input, char ch);
static char *ReadToken(FILE *input, char ch);
static const lisp_object *interpretToken(const char *token);
static const lisp_object **ReadDelimitedList(FILE *input, char delim, size_t *const count /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */);

// Macros
static const lisp_object *CharReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *ListReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *VectorReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *MapReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *StringReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *UnmatchedParenReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *WrappingReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *CommentReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);
static const lisp_object *MetaReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */);

void init_reader() {
	printf("In init_reader\n");
	fflush(stdout);

	macros['\\'] = CharReader;
	macros['"']  = StringReader;
	macros['(']  = ListReader;
	macros[')']  = UnmatchedParenReader;
	macros['[']  = VectorReader;
	macros[']']  = UnmatchedParenReader;
	macros['{']  = MapReader;
	macros['}']  = UnmatchedParenReader;
	macros['\''] = WrappingReader;
	// macros['@']  = WrappingReader;
	macros[';']  = CommentReader;
	macros['^'] = MetaReader;

	// macros['`'] = new SyntaxQuoteReader();
	// macros['~'] = new UnquoteReader();
	// macros['%'] = new ArgReader();
	// macros['#'] = new DispatchReader();

	const char *const pattern = ":?\\([^[:digit:]/].*/\\)?\\(/|[^[:digit:]/][^/]*\\)";		// [:]?([\\D&&[^/]].*/)?(/|[\\D&&[^/]][^/]*)
	int i = regcomp(&symbol_regex, pattern, 0);
	if(i) {
		fprintf(stderr, "Unable to compile regex pattern: %s\n", pattern);
		exit(i);
	}

}

const lisp_object *read(FILE *input, bool EOF_is_error, char return_on /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */) {
	int ch = getc(input);

	while(true) {
		while(isspace(ch)){
			ch = getc(input);
		}

		if(ch == EOF) {
			if(EOF_is_error) {
				exception e = {RuntimeException, "EOF while reading."};
				Raise(e);
			}
			return &EOF_lisp_object;
		}

		if(ch == return_on) {
			return &DONE_lisp_object;
		}

		MacroFn macro = get_macro(ch);
		if(macro) {
			const lisp_object *ret = macro(input, (char) ch);
			if(ret->type == NOOP_type) {
				continue;
			}
			return ret;
		}

		if(ch == '+' || ch == '-') {
			int ch2 = getc(input);
			if(isdigit(ch2)) {
				ungetc(ch2, input);
				return ReadNumber(input, ch);
			}
		}

		if(isdigit(ch)) {
			return ReadNumber(input, ch);
		}

		char* token = ReadToken(input, ch);
		printf("token = %s\n", token);
		fflush(stdout);
		const lisp_object *ret = interpretToken(token);
		return ret;
	}
}

static MacroFn get_macro(int ch) {
	if((size_t)ch < sizeof(macros)/sizeof(*macros)) {
		return macros[ch];
	}
	return NULL;
}

static bool isMacro(int ch) {
	return ((size_t)ch < sizeof(macros)/sizeof(*macros)) && (macros[ch] != NULL);
}

static bool isTerminatingMacro(int ch) {
	return ch != '#' && ch != '\'' && ch != '%' && isMacro(ch);
}

static lisp_object* ReadNumber(FILE *input, char ch) {
	size_t i = 1;
	size_t size = 256;
	char *buffer = GC_MALLOC_ATOMIC(size * sizeof(*buffer));
	assert(buffer);

	for(buffer[0] = ch; true; size *= 2) {
		for( ;i<size; i++) {
			int ch2 = getc(input);
			if(ch2 == EOF || isspace(ch2) || isMacro(ch2)) {
				ungetc(ch2, input);
				char *endptr = NULL;
				errno = 0;
				buffer[i] = '\0';
				long ret_l = strtol(buffer, &endptr, 0);
				if(errno == 0 && endptr == buffer + i) {
					lisp_object *ret = (lisp_object*) NewInteger(ret_l);
					return ret;
				}
				errno = 0;
				double ret_d = strtod(buffer, &endptr);
				if(errno == 0 && endptr == buffer + i) {
					lisp_object *ret = (lisp_object*) NewFloat(ret_d);
					return ret;
				}
				exception e = {NumberFormatException, WriteString(AddString(AddString(NewStringWriter(), "Invalid number: "), buffer))};
				Raise(e);
			}
			buffer[i] = ch2;
		}
		buffer = GC_REALLOC(buffer, 2*size);
		assert(buffer);
	}
}

static char *ReadToken(FILE *input, char ch) {
	size_t i = 1;
	size_t size = 256;
	char *buffer = GC_MALLOC_ATOMIC(size * sizeof(*buffer));
	assert(buffer);

	for(buffer[0] = ch; true; size *= 2) {
		for( ;i<size; i++) {
			int ch2 = getc(input);
			if(ch2 == EOF || isspace(ch2) || isTerminatingMacro(ch2)) {
				ungetc(ch2, input);
				buffer[i] = '\0';
				return buffer;
			}
			buffer[i] = ch2;
		}
		buffer = GC_REALLOC(buffer, 2*size);
		assert(buffer);
	}
}

static const lisp_object *matchSymbol(const char *token) {
	bool isKeyword = token[0] == ':';
	bool nsQualified = false;

	const char *n = token + (isKeyword?1:0);
	const char *ptr;
	while( (ptr = strchr(n, '/')) ) {
		n = ptr + 1;
		nsQualified = true;
	}
	size_t namelen = strlen(n);
	char *name = GC_MALLOC((namelen + 1) * sizeof(*name));
	strncpy(name, n, namelen);
	name[namelen] = '\0';
	printf("n = %p\n", n);
	printf("token = %p\n", token);
	size_t nslen = n - token - (isKeyword ? 1 : 0) - (nsQualified?1:0);
	char *ns = GC_MALLOC((nslen + 1) * sizeof(*ns));
	strncpy(ns, token + (isKeyword?1:0), nslen);
	ns[nslen] = '\0';

	printf("namelen = %zd\n", namelen);
	printf("nslen = %zd\n", nslen);
	printf("name = %s\n", name);
	printf("ns = %s\n", ns);
	fflush(stdout);

	if(nslen > 1 && ns[nslen-2] == ':' && ns[nslen-1] == '/')
		return NULL;
	if(namelen > 0 && name[namelen-1] == ':')
		return NULL;
	if(strstr(token, "::"))
		return NULL;

	if(strlen(token) > 1 && token[0] == ':' && token[1] == ':') {
		const Symbol *ks = internSymbol1(token + 2);
		const Namespace *kns = NULL;
		if(getNamespaceSymbol(ks)) {
			kns = namespaceFor(CurrentNS(), ks);
		} else {
			kns = CurrentNS();
		}
		if(kns)
			return (lisp_object*) internKeyword2(getNameSymbol(getNameNamespace(kns)), getNameSymbol(ks));
		return NULL;
	}

	const Symbol *sym = internSymbol1(token + (isKeyword ? 1 : 0));
	if(isKeyword)
		return (lisp_object*) internKeyword(sym);
	return (lisp_object*)sym;
}

static const lisp_object *interpretToken(const char *token) {
	if(strcmp("nil", token) == 0)
		return NULL;
	if(strcmp("true", token) == 0)
		return (lisp_object*) True;
	if(strcmp("false", token) == 0)
		return (lisp_object*) False;

	const lisp_object *ret = matchSymbol(token);
	if(ret)
		return ret;

	StringWriter *sw = NewStringWriter();
	AddString(sw, "Invalid token: ");
	AddString(sw, token);
	exception e = {RuntimeException, WriteString(sw)};
	Raise(e);
	__builtin_unreachable();
}

static const lisp_object *CharReader(FILE* input, __attribute__((unused)) char c /* *lisp_object opts, *lisp_object pendingForms */) {
	char ch = getc(input);
	if(ch == EOF) {
		exception e = {RuntimeException, "EOF while reading character"};
		Raise(e);
	}

	char *token = ReadToken(input, ch);
	if(strlen(token) == 1)
		return (lisp_object*)NewChar(token[0]);
	if(strcmp(token, "newline") == 0)
		return (lisp_object*)NewChar('\n');
	if(strcmp(token, "space") == 0)
		return (lisp_object*)NewChar(' ');
	if(strcmp(token, "tab") == 0)
		return (lisp_object*)NewChar('\t');
	if(strcmp(token, "backspace") == 0)
		return (lisp_object*)NewChar('\b');
	if(strcmp(token, "formfeed") == 0)
		return (lisp_object*)NewChar('\f');
	if(strcmp(token, "return") == 0)
		return (lisp_object*)NewChar('\r');

	exception e = {RuntimeException, WriteString(AddString(AddString(NewStringWriter(), "Unsupported character: \\"), token))};
	Raise(e);
	__builtin_unreachable();
}

static const lisp_object *StringReader(FILE* input, __attribute__((unused)) char c /* *lisp_object opts, *lisp_object pendingForms */) {
	size_t size = 256;
	size_t i = 0;
	char *str = GC_MALLOC_ATOMIC(size * sizeof(*str));
	assert(str);

	for(int ch = getc(input); ch != '"'; ch = getc(input)) {
		if(ch == EOF) {
			exception e = {RuntimeException, "EOF while reading string."};
			Raise(e);
		}
		if(ch == '\\') {
			ch = getc(input);
			if(ch == EOF) {
				exception e = {RuntimeException, "EOF while reading string."};
				Raise(e);
			}

			switch(ch) {
				case 't':
					ch = '\t';
					break;
				case 'r':
					ch = '\r';
					break;
				case 'n':
					ch = '\n';
					break;
				case 'b':
					ch = '\b';
					break;
				case 'f':
					ch = '\f';
					break;
				case '\\':
				case '"':
					break;
				default: {
							 exception e = {RuntimeException, WriteString(AddChar(AddString(NewStringWriter(),
											 "Unsupported Escape character: \\"), ch))};
							 Raise(e);
						 }
			}
		}
		if(i == size) {
			size *= 2;
			str = GC_REALLOC(str, size);
			assert(str);
		}
		str[i++] = ch;
	}
	str[i++] = '\0';
	lisp_object *obj = (lisp_object*) NewString(str);
	return obj;
}

static const lisp_object **ReadDelimitedList(FILE *input, char delim, size_t *const count /* boolean isRecursive, *lisp_object opts, *lisp_object pendingForms */) {
	size_t size = 8;
	size_t i = 0;
	const lisp_object **ret = GC_MALLOC(size * sizeof(*ret));
	assert(ret);

	while(true) {
		const lisp_object *form = read(input, true, delim);
		if(form->type == EOF_type) {
			exception e = {RuntimeException, "EOF while reading"};
			Raise(e);
		}
		if(form == &DONE_lisp_object) {
			*count = i;
			return ret;
		}
		if(size == i) {
			size *= 2;
			ret = GC_REALLOC(ret, size * sizeof(*ret));
			assert(ret);
		}
		ret[i++] = form;
	}
}

static const lisp_object *ListReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	size_t count;
	const lisp_object **list = ReadDelimitedList(input, ')', &count);
	if(count == 0) return (lisp_object*)EmptyList;
	return (lisp_object*)CreateList(count, list);
}

static const lisp_object *VectorReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	size_t count;
	const lisp_object **list = ReadDelimitedList(input, ']', &count);
	return (lisp_object*) CreateVector(count, list);
}

static const lisp_object *MapReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	size_t count;
	const lisp_object **list = ReadDelimitedList(input, '}', &count);
	if(count & 1) {
		exception e = {RuntimeException, "Map literal must contain an even number of forms"};
		Raise(e);
	}
	return (lisp_object*)CreateHashMap(count, list);
}

static const lisp_object *UnmatchedParenReader(__attribute__((unused)) FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	exception e = {RuntimeException, WriteString(AddChar(AddString(NewStringWriter(), "Unmatched delimiter: "), ch))};
	Raise(e);
	__builtin_unreachable();
}

static const lisp_object *WrappingReader(FILE* input, char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	const Symbol *sym = ch == '\'' ? quoteSymbol : 
		ch == '@' ? derefSymbol : NULL;
	const lisp_object *o = read(input, true, '\0');
	return (lisp_object*) listStar2((lisp_object*)sym, o, NULL);
}

static const lisp_object *CommentReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	for(int ch2 = 0; ch2 != EOF && ch2 != '\n' && ch2 != '\r'; ch2 = getc(input)) {}
	return &NOOP_lisp_object;
}

static const lisp_object *MetaReader(FILE* input, __attribute__((unused)) char ch /* *lisp_object opts, *lisp_object pendingForms */) {
	const lisp_object *o = read(input, true, '\0');
	const IMap *meta = (IMap*) EmptyHashMap;
	switch(o->type) {
		case SYMBOL_type:
		case STRING_type:
			meta = meta->obj.fns->IMapFns->assoc(meta, (lisp_object*)tagKW, o);
			break;
		case KEYWORD_type:
			meta = meta->obj.fns->IMapFns->assoc(meta, o, (lisp_object*)True);
			break;
		case HASHMAP_type:
			break;
		default: {
					 exception e = {IllegalArgumentException, "Metadata must be Symbol,Keyword,String or Map"};
					 Raise(e);
				 }
	}

	o = read(input, true, '\0');
	const IMap *ometa = o->meta;
	for(const ISeq *s = seq((lisp_object*)meta); s != NULL; s = s->obj.fns->ISeqFns->next(s)) {
		const MapEntry *kv = (MapEntry*) s->obj.fns->ISeqFns->first(s);
		ometa = ometa->obj.fns->IMapFns->assoc(ometa, kv->key, kv->val);
	}
	return withMeta(o, ometa);
}
