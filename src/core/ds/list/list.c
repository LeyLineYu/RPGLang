#include "ds/list/list.h"
#include "ds/hashtable/entry.h"
#include "logger/logger.h"
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

const char LIST_UNIT_CANARY[] = "@DDPLEDOCKACIOUS";
static const size_t LIST_UNIT_CANARY_LEN = sizeof(LIST_UNIT_CANARY) - 1;

static const size_t REALLOC_MULT = 2;

static ListIndex  linkNewUnit(List* lst, ListIndex prev, ListIndex next, void* value);
static Error reallocateList(List* lst, size_t newCapacity);

Error listInit(List* lst, size_t initialCapacity, 
               size_t itemSize, free_f freeFunc,
               printf_f printfFunc, cmp_f cmpFunc,
               bool isDoubleLinked) {
  if (!lst || 
      !initialCapacity ||
      !printfFunc ||
      !cmpFunc ||
      !itemSize)
    return BadArgs;
  if (lst->initialized)
    return DenyReinit;
  Error err = listVerify(lst);
  if (err != OK &&
      err != Uninitialized)
    return err;

  /// +1 for the fictional 0th element
  size_t actualCapacity = initialCapacity + 1;
  void*      tempDataPtr = calloc(actualCapacity, itemSize);
  ListIndex* tempNextPtr = (ListIndex*)calloc(actualCapacity, sizeof(ListIndex));
  ListIndex* tempPrevPtr = isDoubleLinked
                           ? (ListIndex*)calloc(actualCapacity, sizeof(ListIndex))
                           : NULL;
  if (!tempDataPtr || !tempNextPtr || 
      (isDoubleLinked && !tempPrevPtr)) {
    free(tempDataPtr); free(tempNextPtr); free(tempPrevPtr);
    return FailMemoryAllocation;
  }

  *lst = (List){
    .capacity = actualCapacity,
    .itemSize = itemSize,
    .data = tempDataPtr,
    .next = tempNextPtr,
    .prev = tempPrevPtr,
    .free = 1,
    .freeTail = actualCapacity - 1,
    .freeFunc   = freeFunc,
    .cmpFunc    = cmpFunc,
    .printfFunc = printfFunc,
    .status = OK,
    .isDoubleLinked = isDoubleLinked,
    .initialized = true, 
  };

  for (size_t written = 0; written < itemSize;) {
    size_t count = MIN(LIST_UNIT_CANARY_LEN, itemSize - written);
    memcpy((char*)lst->data + written, LIST_UNIT_CANARY, count);
    written += count;
  }
  for (ListIndex i = 2; i < actualCapacity; i++)
    lst->next[i - 1] = i;

  return OK;
}

List* listAlloc(size_t initialCapacity, size_t itemSize, 
                free_f freeFunc, printf_f printfFunc, 
                cmp_f cmpFunc, bool isDoubleLinked, Error* status) {
  if (!initialCapacity ||
      !itemSize ||
      !printfFunc ||
      !cmpFunc)
    RETURN_WITH_STATUS(BadArgs, NULL);

  List* lst = (List*)calloc(1, sizeof(List));
  if (!lst)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);

  Error err = OK;
  if ((err = listInit(lst, initialCapacity, itemSize, 
                      freeFunc, printfFunc, 
                      cmpFunc, isDoubleLinked))) {
    free(lst);
    RETURN_WITH_STATUS(err, NULL);
  }

  return lst;
}

#define IS_INVALID_INDEX(index) \
  (index > lst->capacity ||     \
   (lst->isDoubleLinked && index != lst->next[0] && lst->prev[index] == 0))

#define VERIFY_LIST()              \
  if (!lst)                        \
    RETURN_WITH_STATUS(BadArgs, 0) \
  Error err = listVerify(lst);     \
  if (err)                         \
    RETURN_WITH_STATUS(err, 0);

ListIndex listAddAfter(List* lst, ListIndex index, void* value, Error* status) {
  VERIFY_LIST();
  if (!value ||
      IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  if (lst->free == 0 && 
      (err = reallocateList(lst, lst->capacity * REALLOC_MULT - 1)))
    RETURN_WITH_STATUS(err, 0);

  return linkNewUnit(lst, index, lst->next[index], value);
}

ListIndex listAddBefore(List* lst, ListIndex index, void* value, Error* status) {
  VERIFY_LIST();
  if (!lst->isDoubleLinked ||
      !value ||
      IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  if (lst->free == 0 && 
      (err = reallocateList(lst, lst->capacity * REALLOC_MULT - 1)))
    RETURN_WITH_STATUS(err, 0);

  return linkNewUnit(lst, lst->prev[index], index, value);
}

ListIndex listAddAfterTail(List* lst, void* value, Error* status) {
  return listAddAfter(lst, lst->prev[0], value, status);
}

ListIndex listAddAfterHead(List* lst, void* value, Error* status) {
  return listAddAfter(lst, lst->next[0], value, status);
}

ListIndex listAddBeforeTail(List* lst, void* value, Error* status) {
  return listAddBefore(lst, lst->prev[0], value, status);
}

ListIndex listAddBeforeHead(List* lst, void* value, Error* status) {
  return listAddBefore(lst, lst->next[0], value, status);
}

Error listDelete(List* lst, ListIndex index) {
  if (!lst)
    return BadArgs;
  Error err = listVerify(lst);
  if (err)
    return err;
  if (!lst->isDoubleLinked)
    return BadArgs;
  if (IS_INVALID_INDEX(index) ||
      index == 0)
    return BadArgs;

  lst->next[lst->prev[index]] = lst->next[index];
  lst->prev[lst->next[index]] = lst->prev[index];

  if (lst->freeFunc)
    lst->freeFunc(listGet(lst, index));
  memset(listGet(lst, index), 0, lst->itemSize);

  ListIndex oldFree = lst->free;
  lst->free = index;
  if (!lst->freeTail)
    lst->freeTail = index;
  lst->next[index] = oldFree;
  lst->prev[index] = 0;

  return OK;
}

Error listDeleteHead(List* lst) {
  return listDelete(lst, lst->next[0]);
}

Error listDeleteTail(List* lst) {
  return listDelete(lst, lst->prev[0]);
}

Error listDestroy(List* lst, bool isAlloced) {
  if (!lst)
    return BadArgs;

  if (lst->freeFunc)
    freeArray(listGet(lst, 1), listGetCapacity(lst, NULL), 
              lst->itemSize, lst->freeFunc);
  free(lst->data);
  free(lst->next);
  free(lst->prev);

  *lst = (List){};

  if (isAlloced)
    free(lst);

  return OK;
}

ListIndex listGetHead(List* lst, Error* status) {
  return listGetNext(lst, 0, status);
}

ListIndex listGetTail(List* lst, Error* status) {
  return listGetPrev(lst, 0, status);
}

size_t listGetCapacity(List* lst, Error* status) {
  VERIFY_LIST();

  return lst->capacity - 1;
}

ListIndex listGetPrev(List* lst, ListIndex index, Error* status) {
  VERIFY_LIST();
  if (!lst->isDoubleLinked ||
      IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  return lst->prev[index];
}

ListIndex listGetNext(List* lst, ListIndex index, Error* status) {
  VERIFY_LIST();
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, 0);

  return lst->next[index];
}

void* listGetValue(List* lst, ListIndex index, Error* status) {
  Error err = listVerify(lst);
  if (err)
    RETURN_WITH_STATUS(err, NULL);
  if (IS_INVALID_INDEX(index))
    RETURN_WITH_STATUS(BadArgs, NULL);

  return listGet(lst, index);
}

#undef VERIFY_LIST

Error listSetValue(List* lst, ListIndex index, void* value) {
  Error err = listVerify(lst);
  if (err)
    return err;
  if (!value ||
      IS_INVALID_INDEX(index))
    return BadArgs;

  if (lst->freeFunc)
    lst->freeFunc(listGet(lst, index));
  memcpy(listGet(lst, index), value, lst->itemSize);

  return OK;
}

#ifdef _DEBUG
#define CHECK(condition, newStatus)   \
  {                                   \
    if (condition)                    \
    return (lst->status = newStatus); \
  }

Error listVerify(List* lst) {
  if (!lst)
    return BadArgs;
  if (!lst->initialized)
    return Uninitialized;
  if (!lst->capacity ||
      !lst->itemSize)
    return ZeroSize;
  CHECK(!lst->data || !lst->next ||
        (lst->isDoubleLinked && !lst->prev), 
        NullPointerField);
  CHECK(!lst->printfFunc || 
        !lst->cmpFunc, 
        NullPointerField);
  CHECK(lst->prev[0]  > lst->capacity ||
        lst->next[0]  > lst->capacity ||
        lst->free     > lst->capacity ||
        lst->freeTail > lst->capacity, 
        BadIndex);
  for (size_t compared = 0; compared < lst->itemSize;) {
    size_t count = MIN(LIST_UNIT_CANARY_LEN, lst->itemSize - compared);
    if (memcmp((char*)lst->data + compared, LIST_UNIT_CANARY, count) != 0) {
      return (lst->status = CorruptedCanary);
    }
    compared += count;
  }
  return (lst->status = OK);
}

#undef CHECK

#define RETURN_IF_LOOPED(index)               \
{                                             \
  if (beenVisited[cur]) {                     \
    free(beenVisited);                        \
    return (lst->status = LoopedConnections); \
  }                                           \
  beenVisited[cur] = true;                    \
  totalVisited++;                             \
}

Error listLoopCheck(List* lst) {
  Error err = listVerify(lst);
  if (err)
    return err;

  bool* beenVisited = (bool*)calloc(lst->capacity, sizeof(bool));
  if (!beenVisited)
    return FailMemoryAllocation;
  uint64_t totalVisited = 0;
  if (lst->next[0]) {
    for (ListIndex cur = lst->next[0];
         lst->next[cur] < lst->capacity;
         cur = lst->next[cur]) {
      RETURN_IF_LOOPED(cur);
      if (!lst->next[cur])
        break;
    }
  }
  if (lst->free) {
    for (ListIndex cur = lst->free;
         lst->next[cur] < lst->capacity;
         cur = lst->next[cur]) {
      RETURN_IF_LOOPED(cur);
      if (!lst->next[cur])
        break;
    }
  }

  free(beenVisited);
  return totalVisited == lst->capacity - 1
         ? OK
         : (lst->status = DanglingUnit);
}

#undef RETURN_IF_LOOPED
#endif //_DEBUG

Error listLinearize(List* lst) {
  if (!lst)
    return BadArgs;
  Error err = listVerify(lst);
  if (err)
    return err;

  List temp = (List){};
  if ((err = listInit(&temp, listGetCapacity(lst, NULL), 
                      lst->itemSize, lst->freeFunc, 
                      lst->printfFunc, lst->cmpFunc, lst->isDoubleLinked))) {
    listDestroy(&temp, false);
    return err;
  }

  for (ListIndex cur = lst->next[0];
       lst->next[cur] < lst->capacity;
       cur = lst->next[cur]) {
    listAddAfterTail(&temp, listGet(lst, cur), &err);
    if (err) {
      listDestroy(&temp, false);
      return err;
    }
    if (!lst->next[cur])
      break;
  }

  listDestroy(lst, false);
  *lst = temp;

  return OK;
}

static Error reallocateList(List* lst, size_t newCapacity) {
  assert(lst);
  assert(newCapacity > lst->capacity);

  void*      tempDataPtr = realloc(lst->data, newCapacity * lst->itemSize);
  ListIndex* tempNextPtr = (ListIndex*)realloc(lst->next, newCapacity * sizeof(ListIndex));
  ListIndex* tempPrevPtr = lst->isDoubleLinked
                           ? (ListIndex*)realloc(lst->prev, newCapacity * sizeof(ListIndex))
                           : NULL;
  if (!tempDataPtr || !tempNextPtr || 
      (lst->isDoubleLinked && !tempPrevPtr))
    return FailMemoryReallocation;

  lst->data = tempDataPtr;
  lst->next = tempNextPtr;
  lst->prev = tempPrevPtr;

  if (lst->free == 0) { // if no free units left
    lst->free = lst->capacity;
  } else {
    lst->next[lst->freeTail] = lst->capacity;
  }
  lst->freeTail = newCapacity - 1;

  // initialize new free elements (their data is already zero though)
  for (ListIndex i = lst->capacity; i < newCapacity; i++) {
    if (lst->isDoubleLinked)
      lst->prev[i] = 0;
    if (i != lst->capacity)
      lst->next[i - 1] = i;
  }
  lst->next[newCapacity - 1] = 0;

  lst->capacity = newCapacity;

  return OK;
}

static ListIndex linkNewUnit(List* lst, ListIndex prev, 
                             ListIndex next, void* value) {
  assert(lst);

  ListIndex center = lst->free;

  lst->free = lst->next[center];
  if (center == lst->freeTail)
    lst->freeTail = 0;

  lst->next[center] = next;
  lst->next[prev]   = center;
  if (lst->isDoubleLinked) {
    lst->prev[center] = prev;
    lst->prev[next]   = center;
  }
  listSetValue(lst, center, value);

  return center;
}

#undef IS_INVALID_INDEX
