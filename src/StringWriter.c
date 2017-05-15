#include "StringWriter.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct StringWriterStruct {
	char *buffer;
	size_t buffer_size;
	size_t len;
};

StringWriter *NewStringWriter(void) {
	StringWriter *ret = malloc(sizeof(*ret));
	ret->buffer_size = 256;
	ret->buffer = malloc(ret->buffer_size * sizeof(char*));
	ret->len = 0;

	return ret;
}

static void AddStringLen(StringWriter *sw, const char *const str, size_t len) {
	size_t growth;
	for(growth = 1; sw->len + len + 2 >= growth * sw->buffer_size; growth *= 2);
	if(growth > 1) {
	    sw->buffer_size *= growth;
	    sw->buffer = realloc(sw->buffer, sw->buffer_size * sizeof(*(sw->buffer)));
	}
	strncpy(sw->buffer + sw->len, str, len+1);
	sw->len += len;
	sw->buffer[sw->len] = '\0';
}

void AddString(StringWriter *sw, const char *const str) {
	size_t len = strlen(str);
	AddStringLen(sw, str, len);
}

void AddChar(StringWriter *sw, char c) {
	AddStringLen(sw, &c, 1);
}

void Shrink(StringWriter *sw, size_t n) {
	sw->len = sw->len < n ? 0: sw->len - n;
	sw->buffer[sw->len] = '\0';
}

char *WriteString(StringWriter *sw) {
	return sw->buffer;
}
