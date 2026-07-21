#ifndef FRONTEND_H
#define FRONTEND_H

#include "symbol/symbol.h"

static const size_t LEXER_INIT_CAP = 1 << 8;

Error frontend(TranslationUnit* trUnit, MappedFile inputFile);

#endif
