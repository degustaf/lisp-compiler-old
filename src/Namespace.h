#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "LispObject.h"
#include "Symbol.h"

typedef struct Namespace_Struct Namespace;

#include "Var.h"

Namespace *findOrCreateNS(const Symbol *s);

Var *internNS(Namespace *ns, const Symbol *s);

#endif /* NAMESPACE_H */
