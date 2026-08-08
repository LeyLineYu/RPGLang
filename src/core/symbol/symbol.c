#include "symbol/symbol.h"
#include <stdlib.h>
#include <string.h>

bool cmpSymbol(void* symA, void* symB) {
  if ((!symA &&  symB) ||
      ( symA && !symB))
    return false;
  if (!symA && !symB)
    return true;

  Symbol* a = symA;
  Symbol* b = symB;
  return a->argc == b->argc &&
         a->varc == b->varc &&
         a->external == b->external &&
         a->hasReturnValue == b->hasReturnValue &&
         a->mangledName.size == b->mangledName.size &&
         strncmp(a->mangledName.data, b->mangledName.data, a->mangledName.size);
}

void printSymbol(FILE* sink, void* sym) {
  if (!sink || !sym)
    return;

  Symbol* s = sym;
  fprintf(sink, 
          "mangledName = %.*s\n"
          "argc = %lu\n"
          "varc = %lu\n"
          "external = %s\n"
          "hasReturnValue = %s\n",
          (int)s->mangledName.size, s->mangledName.data,
          s->argc, 
          s->varc,
          s->external       ? "true" : "false",
          s->hasReturnValue ? "true" : "false");
}

void freeSymbol(void* sym) {
  if (!sym)
    return;

  Symbol* s = sym;
  free(s->mangledName.data);
}
