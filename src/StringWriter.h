#ifndef STRINGWRITER_H
#define STRINGWRITER_H

#include <stddef.h>

typedef struct StringWriterStruct StringWriter;

StringWriter *NewStringWriter(void);
StringWriter *AddString(StringWriter *sw, const char *const str);
StringWriter *AddChar(StringWriter *sw, char c);
StringWriter *AddInt(StringWriter *sw, int i);
StringWriter *AddFloat(StringWriter *sw, double x);
StringWriter *Shrink(StringWriter *sw, size_t n);
char *WriteString(StringWriter *sw);

#endif /* STRINGWRITER_H */
