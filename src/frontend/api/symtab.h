#ifndef SYMTAB_H
#define SYMTAB_H

#include "symbol/symbol.h"

Error symtabInit(TranslationUnit* trUnit, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc);

bool symtabCheckCalls(TranslationUnit* trUnit, Error* status);

#endif
