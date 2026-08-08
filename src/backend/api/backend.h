#ifndef BACKEND_H
#define BACKEND_H

#include "symbol/symbol.h"

Error backend(const char* outputFilepath, TranslationUnit* trUnit,
              bool stopAtNasm, bool keepTempFiles);

#endif
