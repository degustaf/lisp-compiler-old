#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "LispObject.h"
#include "Symbol.h"

typedef struct Namespace_Struct Namespace;

#include "Var.h"

Namespace *findNS(const Symbol *s);
Namespace *findOrCreateNS(const Symbol *s);
Namespace *lookupAlias(Namespace *ns, const Symbol *alias);
Namespace* namespaceFor(Namespace *inns, const Symbol *sym);

const Symbol* getNameNamespace(const Namespace*);
Var *internNS(Namespace *ns, const Symbol *s);
Var *findInternedVar(const Namespace *ns, const Symbol *s);
const lisp_object* getMapping(const Namespace *ns, const Symbol *s);

const char* toStringMapping(const Namespace *ns);

#endif /* NAMESPACE_H */
