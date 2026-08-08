#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include "error/error.h"
#include "utils/utils.h"
#include <assert.h>
#include <stdbool.h>

typedef struct {
  void* items;
  size_t itemSize;
  size_t capacity;
  size_t count;
  free_f freeFunc;
} DynamicArray;

Error dynArrInit(DynamicArray* dynamicArray, 
                 size_t initialCapacity, 
                 size_t itemSize, free_f freeFunc);
DynamicArray* dynArrAlloc(size_t initialCapacity, size_t itemSize,
                          free_f freeFunc, Error* status);
Error dynArrAppend(DynamicArray* dynamicArray, void* elem);
Error dynArrDestroy(DynamicArray* dynamicArray, bool isAlloced);
Error dynArrVerify(DynamicArray* dynamicArray);

inline void* dynArrGet(DynamicArray* dynamicArray, size_t index) {
  return ((char*)((dynamicArray)->items) + index * (dynamicArray)->itemSize);
}

#endif
