#include "StringWriter.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "gc.h"

struct StringWriterStruct {
	char *buffer;
	size_t buffer_size;
	size_t len;
};

StringWriter *NewStringWriter(void) {
	StringWriter *ret = GC_MALLOC(sizeof(*ret));
	ret->buffer_size = 256;
	ret->buffer = GC_MALLOC_ATOMIC(ret->buffer_size * sizeof(char*));
	ret->len = 0;

	return ret;
}

static void AddStringLen(StringWriter *sw, const char *const str, size_t len) {
	size_t growth;
	for(growth = 1; sw->len + len + 2 >= growth * sw->buffer_size; growth *= 2);
	if(growth > 1) {
	    sw->buffer_size *= growth;
	    sw->buffer = GC_REALLOC(sw->buffer, sw->buffer_size * sizeof(*(sw->buffer)));
	}
	strncpy(sw->buffer + sw->len, str, len+1);
	sw->len += len;
	sw->buffer[sw->len] = '\0';
}

StringWriter *AddString(StringWriter *sw, const char *const str) {
	size_t len = strlen(str);
	AddStringLen(sw, str, len);
	return sw;
}

StringWriter *AddChar(StringWriter *sw, char c) {
	AddStringLen(sw, &c, 1);
	return sw;
}

StringWriter *AddInt(StringWriter *sw, int i) {
	size_t len = snprintf(NULL, 0, "%d", i);
	char *s = GC_MALLOC_ATOMIC((len+1)*sizeof(*s));
	snprintf(s, len, "%d", i);
	s[len] = '\0';
	AddStringLen(sw, s, len);
	return sw;
}

StringWriter *AddFloat(StringWriter *sw, double x) {
	size_t len = snprintf(NULL, 0, "%f", x);
	char *s = GC_MALLOC_ATOMIC((len+1)*sizeof(*s));
	snprintf(s, len, "%f", x);
	s[len] = '\0';
	AddStringLen(sw, s, len);
	return sw;
}

StringWriter *Shrink(StringWriter *sw, size_t n) {
	sw->len = sw->len < n ? 0: sw->len - n;
	sw->buffer[sw->len] = '\0';
	return sw;
}

char *WriteString(StringWriter *sw) {
	return sw->buffer;
}
