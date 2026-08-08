#ifndef ERROR_MODULE_DYNAMIC_ARRAY
#define ERROR_MODULE_DYNAMIC_ARRAY

#define DYNAMIC_ARRAY_ERROR_MODULE()       \
  X(DynamicArrayError,                     \
    "DynamicArray Errors",                 \
    "Errors related to dynamic arrays, "   \
    "whether DynamicArray struct itself, " \
    "or any other dynamic arrays on the heap")

#define DYNAMIC_ARRAY_ERROR_LIST()                                            \
  X(BadCount,                                                                 \
    DynamicArrayError,                                                        \
    "count > capacity",                                                       \
    "The count field in DynamicArray"                                         \
    "is greater than the capacity field. "                                    \
    "Potentially the count field has been corrupted or accidentally changed")

#endif
