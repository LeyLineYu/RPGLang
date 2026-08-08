#ifndef ENTRY_H
#define ENTRY_H

#include "ds/list/list.h"
#include "utils/utils.h"
#include <stddef.h>
#include <stdint.h>

/// Generic Entry for HashTable
typedef struct {
  StringView key;
  uint64_t   hash;
  ListIndex  value;
} Entry;

bool cmpEntry(void* entryA, void* entryB);
void printEntry(FILE* sink, void* entry);

#endif
