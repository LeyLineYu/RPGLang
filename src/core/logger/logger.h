#ifndef LOGGER_H
#define LOGGER_H

#include "error/error.h"
#include <stddef.h>
#include <stdio.h>

#define LOG_LEVEL_LIST() \
  X(DEBUG)               \
  X(INFO)                \
  X(WARN)                \
  X(ERROR)               \
  X(FATAL)

typedef enum {
  #define X(enm) enm,
  LOG_LEVEL_LIST()
  #undef X
} LogLevel;

extern const size_t LOG_LEVELS_SIZE; 

typedef struct {
  FILE* sink;
  LogLevel level;
} Logger;

const char* getLogLevelStr(LogLevel level);

/// filename can be NULL, this way logger sets the output to stderr
Error loggerInit(const char* filename, LogLevel level);
void  loggerCloseFile();

#define loglnTraced(level, fmt, ...)             \
  logln_(level, "[%s:%d in %s]: " fmt "\n",       \
         __FILE__, __LINE__, __PRETTY_FUNCTION__ \
         __VA_OPT__(,) __VA_ARGS__)

#ifndef LOG_FORCE_TRACE
#define logln(level, fmt, ...) \
  logln_(level, ": " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#else
#define logln(level, fmt, ...) \
  loglnTraced(level, fmt, __VA_ARGS__)
#endif

Error logln_(LogLevel level, const char* fmt, ...);

#endif
