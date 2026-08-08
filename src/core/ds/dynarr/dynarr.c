#include "ds/dynarr/dynarr.h"
#include <stdlib.h>
#include <string.h>

static const size_t REALLOC_MULT = 2;

DynamicArray* dynArrAlloc(size_t initialCapacity, size_t itemSize, 
                          free_f freeFunc, Error* status) {
  if (!initialCapacity || !itemSize)
    RETURN_WITH_STATUS(BadArgs, NULL);

  DynamicArray* da = (DynamicArray*)calloc(1, sizeof(DynamicArray));
  if (!da)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = dynArrInit(da, initialCapacity, itemSize, freeFunc))) {
    free(da);
    RETURN_WITH_STATUS(err, NULL);
  }

  return da;
}

Error dynArrInit(DynamicArray* da, size_t initialCapacity, 
                 size_t itemSize, free_f freeFunc) {
  if (!initialCapacity ||
      !itemSize ||
      !da)
    return BadArgs;

  void* items = calloc(initialCapacity, itemSize);
  if (!items)
    return FailMemoryAllocation;

  da->items    = items;
  da->itemSize = itemSize;
  da->count    = 0;
  da->capacity = initialCapacity;
  da->freeFunc = freeFunc;

  return OK;
}

Error dynArrDestroy(DynamicArray* da, bool isAlloced) {
  if (!da)
    return BadArgs;

  if (da->freeFunc)
    freeArray(da->items, da->capacity, 
              da->itemSize, da->freeFunc);
  free(da->items);
  if (isAlloced)
    free(da);

  return OK;
}

Error dynArrAppend(DynamicArray* da, void* elem) {
  if (!elem)
    return BadArgs;
  Error err = dynArrVerify(da);
  if (err)
    return err;
 
  if (da->count == da->capacity) {
    size_t newCapacity = da->capacity * REALLOC_MULT;
    void* temp = realloc(da->items, newCapacity * da->itemSize);
    if (!temp)
      return FailMemoryReallocation;
    da->capacity = newCapacity;
    da->items = temp;
  }

  memcpy((char*)da->items + da->itemSize * (da->count++), 
         elem, da->itemSize);

  return OK;
}

Error dynArrVerify(DynamicArray* da) {
  if (!da)
    return BadArgs;
  if (!da->items)
    return NullPointerField;
  if (da->count > da->capacity)
    return BadCount;
  if (!da->capacity ||
      !da->itemSize)
    return ZeroSize;
  return OK;
}
