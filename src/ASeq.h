#ifndef ASEQ_H
#define ASEQ_H

#include <stdbool.h>

#include "LispObject.h"
#include "Interfaces.h"

const ISeq *moreASeq(const ISeq*);
const ISeq *consASeq(const ISeq*, const lisp_object*);
size_t countASeq(const ICollection*);
const ICollection *emptyASeq(void);
bool EquivASeq(const ICollection*, const lisp_object*);
const ISeq *seqASeq(const Seqable*);

#endif /* ASEQ_H */
