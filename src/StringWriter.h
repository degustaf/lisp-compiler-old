#ifndef STRINGWRITER_H
#define STRINGWRITER_H

#include <stddef.h>

typedef struct StringWriterStruct StringWriter;

StringWriter *NewStringWriter(void);
void AddString(StringWriter *sw, const char *const str);
void AddChar(StringWriter *sw, char c);
void Shrink(StringWriter *sw, size_t n);
char *WriteString(StringWriter *sw);

#endif /* STRINGWRITER_H */
