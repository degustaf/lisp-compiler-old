#ifndef AMAP_H
#define AMAP_H

#include "LispObject.h"

#include "Interfaces.h"

typedef struct KeySeq_struct KeySeq;

const KeySeq* CreateKeySeq(const ISeq *seq);
const KeySeq* CreateKeySeqFromMap(const IMap *map);

typedef struct ValSeq_struct ValSeq;

const ValSeq* CreateValSeq(const ISeq *seq);
const ValSeq* CreateValSeqFromMap(const IMap *map);

#endif /* AMAP_H */
