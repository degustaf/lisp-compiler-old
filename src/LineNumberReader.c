#include "LineNumberReader.h"

#include <stdbool.h>
#include <stdio.h>

#include "gc.h"

struct LineNumberReader_struct {
	FILE *in;
	size_t line;
	size_t column;
	bool atLineStart;
	bool prevAtLineStart;
};

static LineNumberReader _IN = {NULL, 0, 0, false, false};
LineNumberReader *const LNR_IN = &_IN;

static void initStream(void) __attribute__((constructor));
static void initStream(void) {
	_IN.in = stdin;
}

LineNumberReader* NewLineNumberReader(const char *restrict pathname) {
	LineNumberReader *ret = GC_MALLOC(sizeof(*ret));
	ret->line = 1;
	ret->column = 1;
	ret->atLineStart = true;
	ret->prevAtLineStart = false;
	ret->in = fopen(pathname, "r");
	return ret;
}

LineNumberReader* MemOpenLineNumberReader(void *restrict buf, size_t size) {
	LineNumberReader *ret = GC_MALLOC(sizeof(*ret));
	ret->line = 0;
	ret->column = 0;
	ret->atLineStart = true;
	ret->prevAtLineStart = false;
	ret->in = fmemopen(buf, size, "r");
	return ret;
}

void closeLineNumberReader(LineNumberReader *r) {
	fclose(r->in);
}

int getcr(LineNumberReader *r) {
	int ret = getc(r->in);
	r->prevAtLineStart = r->atLineStart;

	if(ret == '\n' || ret == EOF) {
		if(r->line) {
			r->line++;
			r->column = 1;
		}
		r->atLineStart = true;
		return ret;
	}

	if(r->column) r->column++;
	r->atLineStart = false;
	return ret;
}

int ungetcr(int c, LineNumberReader *r) {
	if(r->atLineStart && r->line)
		r->line--;
	else if(r->column)
		r->column--;
	r->atLineStart = r->prevAtLineStart;

	return ungetc(c, r->in);
}

size_t getLineNumber(const LineNumberReader *r) {
	return r->line;
}

size_t getColumnNumber(const LineNumberReader *r) {
	return r->column;
}
