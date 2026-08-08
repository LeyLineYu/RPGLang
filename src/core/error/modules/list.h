#ifndef ERROR_MODULE_LINKED_LIST
#define ERROR_MODULE_LINKED_LIST

#define LINKED_LIST_ERROR_MODULE() \
  X(LinkedListError,               \
    "List Errors",                 \
    "Errors related to single and double linked lists")

#define LINKED_LIST_ERROR_LIST()                                              \
  X(CorruptedCanary,                                                          \
    LinkedListError,                                                          \
    "List's canary is corrupted",                                             \
    "List's canary (0th element) is unexpectedly modified")                   \
  X(BadIndex,                                                                 \
    LinkedListError,                                                          \
    "Index is out of bounds",                                                 \
    "Index is greater than list's capacity or it is zero when not permitted") \
  X(LoopedConnections,                                                        \
    LinkedListError,                                                          \
    "List has loops",                                                         \
    "List has a loop inside of it, causing infinite walk")                    \
  X(DanglingUnit,                                                             \
    LinkedListError,                                                          \
    "List has an unreachable element",                                        \
    "List has an unreachable element, by used nor free walk")

#endif
