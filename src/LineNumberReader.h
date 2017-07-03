#ifndef LINENUMBERREADER_H
#define LINENUMBERREADER_H

#include "stddef.h"

typedef struct LineNumberReader_struct LineNumberReader;

extern LineNumberReader *const LNR_IN;

LineNumberReader* NewLineNumberReader(const char *restrict pathname);
LineNumberReader* MemOpenLineNumberReader(void *restrict buf, size_t size);
void closeLineNumberReader(LineNumberReader*);

int getcr(LineNumberReader*);
int ungetcr(int, LineNumberReader*);

size_t getLineNumber(const LineNumberReader*);
size_t getColumnNumber(const LineNumberReader*);

#endif /* LINENUMBERREADER_H */
