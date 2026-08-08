#include "error/error.h"
#include "utils/utils.h"

#undef prettyError

static const ErrorInfo ERRORS[] = {
#define X(enm, mod, shDescr, descr) \
  [enm] = (ErrorInfo){              \
    .str = #enm,                    \
    .shortDesc = shDescr,           \
    .desc = descr,                  \
    .error = enm,                   \
    .module = mod,                  \
  },

  UNITED_ERROR_LIST()
#undef X
};

static const ErrorModuleInfo ERROR_MODULES[] = {
  #define X(enm, shDescr, descr) \
    [enm] = (ErrorModuleInfo){   \
      .module = enm,             \
      .str = #enm,               \
      .shortDesc = shDescr,      \
      .desc = descr              \
    },                                                     
  
  ERROR_MODULE_LIST()
  #undef X
};

static const ErrorInfo UNKNOWN_ERROR = (ErrorInfo) {
  .error = -1,
  .module = GenericError,
  .str       = "UnknownError",
  .shortDesc = "Unknown error",
  .desc      = "An error with such error code is not present in ERRORS[]"
};

static const ErrorModuleInfo UNKNOWN_ERROR_MODULE = (ErrorModuleInfo) {
  .module = -1,
  .str       = "UnknownModule",
  .shortDesc = "Unknown module",
  .desc      = "A module with such value of enum is not present in ERROR_MODULES[]"
};

const size_t ERRORS_SIZE        = sizer(ERRORS);
const size_t ERROR_MODULES_SIZE = sizer(ERROR_MODULES);

const ErrorInfo* parseError(Error e) {
  return (e < 0 || (size_t)e >= ERRORS_SIZE)
         ? &UNKNOWN_ERROR
         : &ERRORS[e];
}

const ErrorModuleInfo* parseErrorModule(ErrorModule m) {
  return (m < 0 || (size_t)m >= ERROR_MODULES_SIZE)
         ? &UNKNOWN_ERROR_MODULE
         : &ERROR_MODULES[m];
}

void prettyError(FILE* sink, Error error, const char* filename, int line) {
  if (!sink ||
      !filename)
    return;

  const ErrorInfo* err = parseError(error);
  fprintf(sink,
          "%s:%d [ERROR]: %s (error code = %d): %s\n",
          filename, line,
          err->str,
          error,
          err->desc);
}

Error dumpErrors(FILE* file) {
  if (!file)
    return BadArgs;

  fprintf(file,
          "Centralized Error System Dump"
          "Total modules: %zu\n"
          "Total errors : %zu\n",
          ERROR_MODULES_SIZE,
          ERRORS_SIZE);
  ErrorModule curModule = GenericError;
  for (int i = 0; (size_t)i < ERRORS_SIZE; i++) {
    const ErrorInfo* err = parseError(i);
    if (err->module != curModule || i == 0) {
      curModule = err->module;
      const ErrorModuleInfo* info = parseErrorModule(curModule);
      fprintf(file,
              "--Error Module #%u - %s--\n"
              "\tshort desc = %s\n"
              "\tdesc = %s\n",
              curModule, info->str,
              info->shortDesc,
              info->desc);
    }

    fprintf(file, 
            "\t%d: %s\n"
            "\t\tshort desc = %s\n"
            "\t\tdesc = %s\n", 
            i, err->str, 
            err->shortDesc, 
            err->desc);
  }
  return OK;
}
