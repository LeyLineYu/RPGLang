#include "ds/hashtable/entry.h"
#include <string.h>

bool cmpEntry(void* entryA, void* entryB) {
  if ((!entryA &&  entryB) ||
      ( entryA && !entryB))
    return false;
  if (!entryA && !entryB)
    return true;

  Entry* a = entryA;
  Entry* b = entryB;
  return a->key.size == b->key.size &&
         a->hash == b->hash &&
         a->value == b->value &&
         strncmp(a->key.data, b->key.data, a->key.size);
}

void printEntry(FILE* sink, void* entry) {
  if (!sink || !entry)
    return;

  Entry* e = entry;
  fprintf(sink, 
          "key = %.*s\n"
          "hash = %lu\n"
          "value = "LIST_INDEX_FMT"\n",
          (int)e->key.size, e->key.data,
          e->hash,
          e->value);
}
