#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "error/error.h"
#include "utils/utils.h"

typedef uint64_t ListIndex;
#define LIST_INDEX_FMT "%lu"

extern const char LIST_UNIT_CANARY[];

typedef struct {
  ListIndex capacity;
  size_t itemSize;
  void*  data;
  ListIndex* next;
  ListIndex* prev;
  ListIndex  free;
  ListIndex  freeTail;
  free_f   freeFunc;
  printf_f printfFunc;
  cmp_f    cmpFunc;
  Error status;
  bool isDoubleLinked;
  bool initialized;
} List;

Error listInit(List* lst, size_t initialCapacity, 
               size_t itemSize, free_f freeFunc,
               printf_f printfFunc, cmp_f cmpFunc,
               bool isDoubleLinked);
List* listAlloc(size_t initialCapacity, size_t itemSize, 
                free_f freeFunc, printf_f printfFunc, 
                cmp_f cmpFunc, bool isDoubleLinked, Error* status);

/// Note 1: If the list is single-linked, 
/// then make sure the passed in index is an actual existing element in the list
/// and not a part of the free area, otherwise some undefined behaviour may arise
/// Note 2: Does not support inserting after fictional 0th element if the list isnt empty
ListIndex  listAddAfter(List* lst, ListIndex index, void* value, Error* status);
ListIndex  listAddAfterHead(List* lst, void* value, Error* status);
ListIndex  listAddAfterTail(List* lst, void* value, Error* status);
/// Note: any addBefore funcs do not support single-linked lists
ListIndex  listAddBefore(List* lst, ListIndex index, void* value, Error* status);
ListIndex  listAddBeforeHead(List* lst, void* value, Error* status);
ListIndex  listAddBeforeTail(List* lst, void* value, Error* status);

/// Note: any delete funcs do not support single-linked lists
Error listDelete(List* lst, ListIndex index);
Error listDeleteHead(List* lst);
Error listDeleteTail(List* lst);

ListIndex  listGetHead(List* lst, Error* status);
ListIndex  listGetTail(List* lst, Error* status);
size_t     listGetCapacity(List* lst, Error* status);
ListIndex  listGetPrev(List* lst, ListIndex index, Error* status);
ListIndex  listGetNext(List* lst, ListIndex index, Error* status);
void*      listGetValue(List* lst, ListIndex index, Error* status);
Error      listSetValue(List* lst, ListIndex index, void* value);

/// Note: O(n) time complexity
Error listLinearize(List* lst);

Error listDestroy(List* lst, bool isAlloced);

#ifdef _DEBUG
  Error listVerify(List* lst);
  /// Note: O(n) time complexity
  Error listLoopCheck(List* lst);
#else
  #define listVerify(lst) OK
  #define listLoopCheck(lst) OK
#endif

/// for getting an address of a certain elem
#define listGet(lst, index) ((char*)((lst)->data) + index * (lst)->itemSize)

#endif
