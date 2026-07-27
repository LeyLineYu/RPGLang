#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>
#include <stdio.h>
#include "error/modules/dynarr.h"
#include "error/modules/generic.h"
#include "error/modules/list.h"

typedef int Error;

// INFO: Use this one when you want to iterate through every single one, since it keeps the same order
#define UNITED_ERROR_LIST()  \
  GENERIC_ERROR_LIST()       \
  DYNAMIC_ARRAY_ERROR_LIST() \
  LINKED_LIST_ERROR_LIST()

#define ERROR_MODULE_LIST()    \
  GENERIC_ERROR_MODULE()       \
  DYNAMIC_ARRAY_ERROR_MODULE() \
  LINKED_LIST_ERROR_MODULE()

typedef enum {
  #define X(enm, ...) enm,
  ERROR_MODULE_LIST()
  #undef X
} ErrorModule;

_Static_assert(GenericError == 0, 
               "GenericError must be 0 to be OK's and Fail's module");

typedef enum {
  #define X(enm, ...) enm,
  UNITED_ERROR_LIST()
  #undef X
} ErrorEnum;

_Static_assert(OK == 0,   "OK must be 0 to pass in if (cond)");
_Static_assert(Fail == 1, "Fail must be 1 to fail in if (cond)");

extern const size_t ERROR_MODULES_SIZE;
extern const size_t ERRORS_SIZE;

typedef struct {
  const char* str;
  const char* shortDesc;
  const char* desc;
  int error;
  ErrorModule module;
} ErrorInfo;

typedef struct {
  const char* str;
  const char* shortDesc;
  const char* desc;
  int module;
} ErrorModuleInfo;

const ErrorInfo* parseError(Error error);
const ErrorModuleInfo* parseErrorModule(ErrorModule module);

void prettyError(FILE* sink, Error error, const char* filename, int line);
#define prettyError(sink, error) prettyError(sink, error, __FILE__, __LINE__)

Error dumpErrors(FILE* file);

#ifndef LOG_STATUSES

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
    *status = value;                           \
  return returnValue;                          \
  }

#else

#include "logger/logger.h"

#define RETURN_WITH_STATUS(value, returnValue) \
  {                                            \
  if (status)                                  \
    *status = value;                           \
  loglnTraced(WARN, "Returned with status %s", \
              parseError(value)->str);         \
  return returnValue;                          \
  }

#endif //LOG_STATUSES

#define DEFER(retVal) \
  {                   \
  exitValue = retVal; \
  goto exit;          \
  }

#endif //ERROR_H
