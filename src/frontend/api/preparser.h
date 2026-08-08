#ifndef PREPARSER_H
#define PREPARSER_H

#include "error/error.h"
#include "frontend/api/lexer.h"

bool preparse(Tokens* ts, Difficulty dif, Error* status);

#endif
