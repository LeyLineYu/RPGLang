#include "logger/logger.h"
#include "utils/utils.h"
#include <stdarg.h>

static const char* const LOG_LEVELS[] = {
  #define X(enm) \
    [enm] = #enm,

  LOG_LEVEL_LIST()
  #undef X
};

const size_t LOG_LEVELS_SIZE = sizer(LOG_LEVELS);

static Logger LOGGER = {
  .sink  = NULL,
  .level = DEBUG,
};

Error loggerInit(const char* filename, LogLevel level) {
  if (LOGGER.sink)
    return DenyReinit; 

  FILE* sink = stderr;
  if (filename) {
    sink = fopen(filename, "a");
    if (!sink)
      return FailFileOpen;

    if (setvbuf(sink, NULL, _IONBF, 0))
      return FileError;
  }

  LOGGER.sink  = sink;
  LOGGER.level = level;
  logln(INFO, "Logger Initialized");
  return OK;
}

void loggerCloseFile() {
  if (LOGGER.sink &&
      LOGGER.sink != stderr) {
    logln(INFO, "Logger waves goodbye\n"
                "--------------------------------");
    fclose(LOGGER.sink);
  }

  LOGGER.sink = NULL;
  return;
}

Error logln_(LogLevel level, const char* fmt, ...) {
    if (!LOGGER.sink)
      return NullPointerField;
    if (level < LOGGER.level)
      return OK;

    char tsBuf[TIMESTAMP_BUF_SZ] = {};
    Error err = OK;
    if ((err = snTimestamp(tsBuf, TIMESTAMP_BUF_SZ)))
      return err;

    fprintf(LOGGER.sink, "[%s][%s]", tsBuf, getLogLevelStr(level));
    va_list args = {};
    va_start(args, fmt);
    vfprintf(LOGGER.sink, fmt, args);
    va_end(args);

    return OK;
}

static const char* UNKNOWN_LOG_LEVEL_STR = "unknown";

const char* getLogLevelStr(LogLevel level) {
  if (level < 0 || level >= LOG_LEVELS_SIZE)
    return UNKNOWN_LOG_LEVEL_STR;

  return LOG_LEVELS[level];
}
