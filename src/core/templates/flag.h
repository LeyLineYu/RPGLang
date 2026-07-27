#ifndef FLAG_H
#define FLAG_H

#include "utils/utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HELP_FIELD
  #define HELP_FIELD help
#endif
#ifndef LONG_FLAG_LIST
  #define LONG_FLAG_LIST() \
    X(bool, help, 'h', false, "Display this message", parseBool)
#endif
#ifndef FLAG_CONTEXT_ADDITIONAL_FIELDS
  #define FLAG_CONTEXT_ADDITIONAL_FIELDS()
#endif
#ifndef FLAG_CONTEXT_PREINIT_HOOK
  #define FLAG_CONTEXT_PREINIT_HOOK()
#endif
#ifndef REQUIRED_ARG_COUNT 
  #define REQUIRED_ARG_COUNT 0
#endif
#ifndef USAGE
  #define USAGE(file)
#endif
#ifndef USAGE_VERBOSE
  #define USAGE_VERBOSE(file)
#endif
#ifndef DEFAULT_ARG_HANDLER
  #define DEFAULT_ARG_HANDLER()
#endif
#ifndef POST_PARSING_HOOK
  #define POST_PARSING_HOOK()
#endif

typedef struct {
#define X(type, longName, ...) type longName;
  LONG_FLAG_LIST()
#undef X
  FLAG_CONTEXT_ADDITIONAL_FIELDS()
} FlagContext;

typedef struct {
  const char* type;
  const char* longName;
  const char* defaultValue;
  const char* desc;
  char  shortName;
} FlagInfo;

static const FlagInfo FLAGS[] = {
#define X(typee, lngName, shrtName, defValue, description, ...) \
  (FlagInfo){                                                   \
    .type = #typee,                                             \
    .longName = #lngName,                                       \
    .shortName = shrtName,                                      \
    .defaultValue = #defValue,                                  \
    .desc = description,                                        \
  },

  LONG_FLAG_LIST()
#undef X
};

static const size_t FLAGS_SIZE = sizer(FLAGS);

void flagContextInit(FlagContext* ctx);
char* popArg(int* argc, char*** argv);
char* peekArg(int* argc, char*** argv);
void  parseArgs(int* argc, char*** argv, FlagContext* ctx);
bool  parseBool(int* argc, char*** argv, 
                FlagContext* ctx, bool* failed);
char* parseString(int* argc, char*** argv, 
                  FlagContext* ctx, bool* failed);
const char* getFlagArgsStr(const char* type);

void flagContextInit(FlagContext* ctx) {
  if (!ctx)
    return;

  FLAG_CONTEXT_PREINIT_HOOK();
  *ctx = (FlagContext){
#define X(type, longName, shortName, defaultValue, ...) .longName = defaultValue,
    LONG_FLAG_LIST()
#undef X
  };
  return;
}

#define PRELUDE()         \
  {                       \
  assert(argc &&          \
         argv && *argv && \
         ctx);            \
  }

#define FAILED() failed = true;

void parseArgs(int* argc, char*** argv, FlagContext* ctx) {
  PRELUDE();

  bool failed = false;
  _unused const char* PROG_NAME = popArg(argc, argv);
  if (*argc < REQUIRED_ARG_COUNT) {
    USAGE(stderr);
    FAILED();
  }

  char* arg = NULL;
  while ((arg = popArg(argc, argv))) {
    if (*arg == '-') {
      arg++;

      if (*arg != '-') {

        for (; *arg; arg++) {
          switch (*arg) {
            #define X(type, longName, shortName, defaultValue, desc, parser)        \
              case shortName:                                                       \
                ctx->longName = parser(argc, argv, ctx, &failed);                   \
                if (failed)                                                         \
                  fprintf(stderr,                                                   \
                          "ERROR: flag '-%c' is missing a required argument: %s\n", \
                          shortName, getFlagArgsStr(#type));                        \
                break;

            LONG_FLAG_LIST()
            #undef X
            case '/':
              continue;
            default: 
              fprintf(stderr, 
                      "ERROR: Unknown flag '-%c', "
                      "try '%s --"str(HELP_FIELD)"' for more information\n", 
                      *arg, PROG_NAME);
              FAILED();
              break;
          }
        }

      } else {
        arg++;
        if (arg[1] == '/')
          continue;

        #define X(type, longName, shortName, defaultValue, desc, parser)            \
          if (strcmp(arg, #longName) == 0) {                                        \
            ctx->longName = parser(argc, argv, ctx, &failed);                       \
            continue;                                                               \
          }

        LONG_FLAG_LIST()
        #undef X

        fprintf(stderr, 
                "ERROR: Unknown flag \"--%s\", "
                "try '%s --"str(HELP_FIELD)"' for more information\n", 
                arg, PROG_NAME);
        FAILED();

      }
    } else {
      DEFAULT_ARG_HANDLER();
    }
  }

  POST_PARSING_HOOK();
  if (ctx->HELP_FIELD) {
    USAGE_VERBOSE(stdout);
    puts("OPTIONS:");
    for (const FlagInfo* i = FLAGS; i < FLAGS + FLAGS_SIZE; i++) {
      printf("\t-%c, --%s %s\t\t\t%s (default: %s)\n",
             i->shortName, i->longName,
             getFlagArgsStr(i->type),
             i->desc, i->defaultValue);
    }
    exit(OK);
  }
  if (failed)
    exit(Fail);
  return;
}

#undef FAILED

/// parses optional true/false/1/0 arg after a bool flag
/// by default returns true
bool parseBool(int* argc, char*** argv, 
               FlagContext* ctx, bool* failed) {
  PRELUDE();
  assert(failed);

  char* peek = peekArg(argc, argv);
  if (!peek)
    return true;

  if (strcmp(peek, "false") == 0 ||
      strcmp(peek, "0")     == 0) {
    peek = popArg(argc, argv); // pop the arg since it is correct
    return false;
  }
  if (strcmp(peek, "true") == 0 ||
      strcmp(peek, "1")    == 0) {
    peek = popArg(argc, argv); 
    return true;
  }

  return true;
}

/// parses required string
char* parseString(int* argc, char*** argv, 
                  FlagContext* ctx, bool* failed) {
  PRELUDE();
  assert(failed);

  char* arg = popArg(argc, argv); 
  if (!arg)
    *failed = true;
  return arg;
}

#undef PRELUDE

char* popArg(int* argc, char*** argv) {
  char* arg = peekArg(argc, argv);
  if (arg) {
    (*argc)--;
    (*argv)++;
  }
  return arg;
}

char* peekArg(int* argc, char*** argv) {
  if (!argc || !*argc ||
      !argv || !*argc)
    return NULL;

  return (*argv)[0];
}

const char* getFlagArgsStr(const char* type) {
  if (!type)
    return NULL;

  if (strcmp(type, "bool")  == 0 ||
      strcmp(type, "_Bool") == 0)
    return "[true|false|1|0]";
  if (strcmp(type, "char*")  == 0 ||
      strcmp(type, "char *") == 0)
    return "<string>";

  return "";
}

#endif
