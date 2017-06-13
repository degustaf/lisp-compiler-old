#ifndef VAR_H
#define VAR_H

#include "LispObject.h"
#include <stdbool.h>
#include "Interfaces.h"

typedef struct Var_struct Var;

#include "Namespace.h"

Var* NewVar(const Namespace *ns, const Symbol *sym, const lisp_object *root);
Var* internVar(Namespace *ns, const Symbol *sym, const lisp_object *root, bool replaceRoot);
Var* createVar(const lisp_object* root);

const lisp_object *getTag(const Var *v);
const lisp_object *getVar(const Var *v);
void setTag(Var *v, const Symbol *obj);
void setMeta(Var *v, const IMap *m);
void setMacro(Var *v);
void setVar(Var *v, const lisp_object *val);
bool isMacroVar(Var *v);
Var *setDynamic(Var *v);
Var *setDynamic1(Var *v, bool b);
bool isDynamic(const Var *v);
bool isBound(const Var *v);
bool isPublic(const Var *v);
const lisp_object* deref(const Var *v);
void bindRoot(Var *v, const lisp_object *obj);


const Namespace *getNamespaceVar(const Var *v);
const Symbol *getSymbolVar(const Var *v);

void pushThreadBindings(const IMap *bindings);
void popThreadBindings(void);

#endif /* VAR_H */
