#ifndef SYMBOL_H
#define SYMBOL_H

#include "LispObject.h"

typedef struct Symbol_struct Symbol;

const Symbol *internSymbol1(const char *nsname);
const Symbol *internSymbol2(const char *ns, const char *name);

const char *getNameSymbol(const Symbol *s);
const char *getNamespaceSymbol(const Symbol *s);

extern const Symbol _arglistsSymbol;
extern const Symbol _ConstSymbol;
extern const Symbol _DefSymbol;
extern const Symbol _DocSymbol;
extern const Symbol _macroSymbol;
extern const Symbol _privateSymbol;
extern const Symbol _quoteSymbol;
extern const Symbol _tagSymbol;

extern const Symbol *const quoteSymbol;

extern const Symbol *const AmpSymbol;
extern const Symbol *const derefSymbol;
extern const Symbol *const DoSymbol;
extern const Symbol *const FNSymbol;
extern const Symbol *const inNamespaceSymbol;
extern const Symbol *const in_nsSymbol;
extern const Symbol *const loadFileSymbol;
extern const Symbol *const namespaceSymbol;
extern const Symbol *const nsSymbol;

#endif /* SYMBOL_H */
