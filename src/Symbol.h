#ifndef SYMBOL_H
#define SYMBOL_H

#include "LispObject.h"

typedef struct Symbol_struct Symbol;

const Symbol *internSymbol1(const char *nsname);
const Symbol *internSymbol2(const char *ns, const char *name);

const char *getNameSymbol(const Symbol *s);
const char *getNamespaceSymbol(const Symbol *s);

extern const Symbol _DefSymbol;
extern const Symbol _DocSymbol;
extern const Symbol _macroSymbol;
extern const Symbol _tagSymbol;

extern const Symbol *const DoSymbol;
extern const Symbol *const FNSymbol;
extern const Symbol *const inNamespaceSymbol;
extern const Symbol *const loadFileSymbol;
extern const Symbol *const namespaceSymbol;

#endif /* SYMBOL_H */
