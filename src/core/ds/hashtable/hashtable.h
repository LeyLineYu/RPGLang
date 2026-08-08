#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "ds/list/list.h"
#include "error/error.h"
#include "utils/utils.h"
#include <stddef.h>
#include <stdint.h>

typedef uint64_t (*hash_f)(StringView strView);

typedef struct {
  List   values;
  List*  buckets;
  size_t bucketCount;
  hash_f hashFunc;
  bool initialized;
} HashTable;

Error hashTableInit(HashTable* table, size_t bucketCount, 
                    size_t initialListCapacity, size_t itemSize, 
                    free_f freeFunc, printf_f printfFunc, 
                    cmp_f cmpFunc, hash_f hashFunc);
HashTable* hashTableAlloc(size_t bucketCount, size_t initialListCapacity,
                          size_t itemSize, free_f freeFunc,
                          printf_f printfFunc, cmp_f cmpFunc,
                          hash_f hashFunc, Error* status);
#define hashTablePut(table, key, value) \
  hashTablePutExt(table, key, value, NULL, NULL)
/// hash table put, extended: 
/// provides bucketIndex and listIndex in that bucket to which it put the value
Error hashTablePutExt(HashTable* table, StringView key, void* value, 
                      size_t* bucketIndex, ListIndex* listIndex);
#define hashTableGet(table, key, result, status) \
  hashTableGetExt(table, key, result, NULL, NULL, status)
bool hashTableGetExt(HashTable* table, StringView key, void* result, 
                     size_t* bucketIndex, ListIndex* listIndex, Error* status);
Error hashTableDelete(HashTable* table, StringView key);
Error hashTableVerify(HashTable* table);
Error hashTableDestroy(HashTable* table, bool isAlloced);

uint64_t hashdjb2(StringView strView);
uint64_t hashRotate(StringView strView);

#endif
