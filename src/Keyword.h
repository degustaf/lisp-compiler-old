#ifndef KEYWORD_H
#define KEYWORD_H

#include "LispObject.h"

#include "Symbol.h"

typedef struct Keyword_struct Keyword;

const Keyword *internKeyword(const Symbol *s);
const Keyword *internKeyword1(const char *nsname);
const Keyword *internKeyword2(const char *ns, const char *name);

const char *getNameKeyword(const Keyword *k);
const char *getNamespaceKeyword(const Keyword *k);

const Keyword *findKeyword(const Symbol *s);
const Keyword *findKeyword1(const char *nsname);
const Keyword *findKeyword2(const char *ns, const char *name);

extern const Keyword *const arglistsKW;
extern const Keyword *const ConstKW;
extern const Keyword *const DocKW;
extern const Keyword *const macroKW;
extern const Keyword *const privateKW;
extern const Keyword *const tagKW;

#endif /* KEYWORD_H */
