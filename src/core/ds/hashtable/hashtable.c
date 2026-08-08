#include "ds/hashtable/hashtable.h"
#include "ds/hashtable/entry.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static ListIndex listFindByKey(List* lst, StringView key);

Error hashTableInit(HashTable* table, size_t bucketCount, 
                    size_t initialListCapacity, size_t itemSize, 
                    free_f freeFunc, printf_f printfFunc, 
                    cmp_f cmpFunc, hash_f hashFunc) {
  Error err = hashTableVerify(table);
  if (err != OK &&
      err != Uninitialized)
    return err;
  if (table->initialized)
    return DenyReinit;
  if (!bucketCount || 
      !initialListCapacity ||
      !itemSize ||
      !printfFunc ||
      !cmpFunc ||
      !hashFunc)
    return BadArgs;

  List* buckets = (List*)calloc(bucketCount, sizeof(List));
  if (!buckets)
    return FailMemoryAllocation;

  for (size_t i = 0; i < bucketCount; i++) {
    if ((err = listInit(buckets + i, initialListCapacity, 
                        sizeof(Entry), NULL, printEntry, cmpEntry, true))) {
        free(buckets);
        return err;
    }
  }

  List values = (List){};
  if ((err = listInit(&values, initialListCapacity * bucketCount,
                      itemSize, freeFunc, printfFunc, cmpFunc, true))) {
    free(buckets);
    return err;
  }

  *table = (HashTable){
    .values = values,
    .buckets = buckets,
    .bucketCount = bucketCount,
    .hashFunc = hashFunc,
    .initialized = true,
  };

  return OK;
}

HashTable* hashTableAlloc(size_t bucketCount, size_t initialListCapacity,
                          size_t itemSize, free_f freeFunc,
                          printf_f printfFunc, cmp_f cmpFunc,
                          hash_f hashFunc, Error* status) {
  if (!bucketCount ||
      !initialListCapacity ||
      !itemSize ||
      !printfFunc ||
      !cmpFunc ||
      !hashFunc)
    RETURN_WITH_STATUS(BadArgs, NULL);

  HashTable* table = (HashTable*)calloc(1, sizeof(HashTable));
  if (!table)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = hashTableInit(table, bucketCount, 
                           initialListCapacity, itemSize, 
                           freeFunc, printfFunc, 
                           cmpFunc, hashFunc))) {
    free(table);
    RETURN_WITH_STATUS(err, NULL);
  }

  return table;
}

Error hashTablePutExt(HashTable* table, StringView key, void* value, 
                      size_t* bucketIndexPtr, ListIndex* listIndexPtr) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    return err;
  if (!key.data || 
      !key.size ||
      !value)
    return BadArgs;
  
  uint64_t hash = table->hashFunc(key);
  size_t bucketIndex = hash % table->bucketCount;
  if (bucketIndexPtr)
    *bucketIndexPtr = bucketIndex;
  List* bucket = table->buckets + (bucketIndex);
  ListIndex index = listFindByKey(bucket, key);
  if (!index) {
    ListIndex valueIndex = listAddAfterTail(&table->values, value, &err);
    if (err)
      return err;
    Entry entry = (Entry) {
      .key = key,
      .value = valueIndex,
      .hash = hash,
    };
    ListIndex listIndex = listAddAfterTail(bucket, &entry, &err);
    if (err)
      return err;
    if (listIndexPtr)
      *listIndexPtr = listIndex;
  } else {
    if (listIndexPtr)
      *listIndexPtr = index;
    Entry* entry = (Entry*)listGetValue(bucket, index, &err);
    if (err)
      return err;
    listSetValue(&table->values, entry->value, value);
  }

  return OK;
}

bool hashTableGetExt(HashTable* table, StringView key, void* result, 
                     size_t* bucketIndexPtr, ListIndex* listIndexPtr, 
                     Error* status) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    RETURN_WITH_STATUS(err, false);
  if (!key.data || 
      !key.size ||
      !result)
    RETURN_WITH_STATUS(BadArgs, false);
  
  uint64_t hash = table->hashFunc(key);
  size_t bucketIndex = hash % table->bucketCount;
  List* bucket = table->buckets + (bucketIndex);  
  ListIndex index = listFindByKey(bucket, key);
  if (!index)
    return false;
  if (listIndexPtr)
    *listIndexPtr = index;
  if (bucketIndexPtr)
    *bucketIndexPtr = bucketIndex;
  Entry* entry = (Entry*)listGetValue(bucket, index, &err);

  *(void**)result = listGetValue(&table->values, entry->value, &err);
  return true;
}

Error hashTableDelete(HashTable* table, StringView key) {
  Error err = OK;
  if ((err = hashTableVerify(table)))
    return err;
  if (!key.data || !key.size)
    return BadArgs;
  
  uint64_t hash = table->hashFunc(key);
  List* bucket = table->buckets + (hash % table->bucketCount);
  ListIndex index = listFindByKey(bucket, key);
  if (index) {
    Entry* entry = (Entry*)listGetValue(bucket, index, &err);
    listDelete(&table->values, entry->value);
    listDelete(bucket, index);
  }

  return OK;
}

Error hashTableVerify(HashTable* table) {
  if (!table)
    return BadArgs;
  if (!table->initialized)
    return Uninitialized;
  if (!table->buckets ||
      !table->hashFunc) 
    return NullPointerField;
  if (!table->bucketCount)
    return ZeroSize;
  Error err = OK;
  for (size_t i = 0; i < table->bucketCount; i++) {
    if ((err = listVerify(table->buckets + i)))
      return err;
  }
  if ((err = listVerify(&table->values)))
    return err;
  return OK;
}

Error hashTableDestroy(HashTable* table, bool isAlloced) {
  if (!table)
    return BadArgs;

  listDestroy(&table->values, false);
  if (table->buckets) {
    for (size_t i = 0; i < table->bucketCount; i++) {
      listDestroy(table->buckets + i, false);
    }
  }
  free(table->buckets);

  *table = (HashTable){};

  if (isAlloced)
    free(table);
  return OK;
}

// NOTE: Mainly taken from: http://www.cse.yorku.ca/~oz/hash.html
_unused uint64_t hashdjb2(StringView strView) {
  uint64_t hash = 5381;

  for (size_t i = 0; i < strView.size; i++)
    hash = ((hash << 5) + hash) + (uint64_t)strView.data[i]; /* hash * 33 + c */

  return hash;
}

static const size_t SHIFT = 23;

_unused uint64_t hashRotate(StringView strView) {
  uint64_t hash = 0;
 
  // NOTE: Source for left rotation: https://stackoverflow.com/a/13289498
  for (size_t i = 0; i < strView.size; i++) {
    hash = (hash << SHIFT) | (hash >> ((sizeof(hash) - SHIFT) % sizeof(hash)));
    hash ^= (uint64_t)strView.data[i];
  }

  return hash;
}

static ListIndex listFindByKey(List* lst, StringView key) {
  assert(lst);
  assert(key.data);
  assert(key.size);

  for (ListIndex cur = lst->next[0];
       lst->next[cur] < lst->capacity;
       cur = lst->next[cur]) {
    if (!cur)
      break;
    Entry* unit = (Entry*)listGetValue(lst, cur, NULL);
    //logln(INFO, "Comparing %.*s and %.*s", key.size, key.data, unit->key.size, unit->key.data);
    if (unit->key.size == key.size &&
        strncmp(unit->key.data, key.data, key.size) == 0) {
      //logln(INFO, "Success");
      return cur;
    }
  }

  //logln(INFO, "Not Found");
  return 0;
}
