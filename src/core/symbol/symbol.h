#ifndef SYMBOL_H
#define SYMBOL_H

#include "ds/hashtable/hashtable.h"
#include "ds/tree/node.h"
#include "utils/utils.h"

typedef struct {
  HashTable symtab;
  TreeNode* ast;
} TranslationUnit;

typedef struct {
  StringView mangledName;
  uint64_t argc;
  uint64_t varc;
  bool hasReturnValue;
  bool external;
} Symbol;

bool cmpSymbol(void* symA, void* symB);
void printSymbol(FILE* sink, void* sym);
void freeSymbol(void* sym);

#endif
