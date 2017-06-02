#ifndef RUNTIME_H
#define RUNTIME_H

#include "LispObject.h"
#include "Namespace.h"
#include "Var.h"

extern Namespace *LISP_ns;
extern Var *OUT;

const Var* RTVar(const char *ns, const char *name);
void initRT(void);

#endif /* RUNTIME_H */
