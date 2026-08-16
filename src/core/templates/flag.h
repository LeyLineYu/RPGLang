/// Uses macro substitution to autogen C code that parses the command-line arguments
#ifndef FLAG_H
#define FLAG_H

#include "utils/utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Signifies what field inside of the FlagContext struct is responsible for help printing
/// Also signifies what flag is used for help (e.g. --help in this case)
#ifndef HELP_FIELD
  #define HELP_FIELD help
#endif
/// List of flags that both have a long variant and a short variant
/// X(type, longName(also doubles as field name), shortName, defaultValue, description, functionToUseForParsing)
#ifndef LONG_FLAG_LIST
  #define LONG_FLAG_LIST() \
    X(bool, help, 'h', false, "Display this message", parseBool)
#endif
/// Any fields you might want to include in FlagContext struct go here (e.g. char*[] inputFiles; int amount)
#ifndef FLAG_CONTEXT_ADDITIONAL_FIELDS
  #define FLAG_CONTEXT_ADDITIONAL_FIELDS()
#endif
/// Any code you want to run before all the default flag values are assigned in flagContextInit function
/// Useful to initialize your additional fields
#ifndef FLAG_CONTEXT_PREINIT_HOOK
  #define FLAG_CONTEXT_PREINIT_HOOK()
#endif
/// How many arguments does the program need (not counting the program name)
#ifndef REQUIRED_ARG_COUNT 
  #define REQUIRED_ARG_COUNT 0
#endif
/// Code to print the consise usage
#ifndef USAGE
  #define USAGE(file)
#endif
/// Code to print verbose usage, like when running --help
#ifndef USAGE_VERBOSE
  #define USAGE_VERBOSE(file)
#endif
/// Code to run when the argument isn't a flag and not a flag's arg
#ifndef DEFAULT_ARG_HANDLER
  #define DEFAULT_ARG_HANDLER()
#endif
/// Code to run after every argument has been parsed (useful to check if the necessary fields were assigned)
#ifndef POST_PARSING_HOOK
  #define POST_PARSING_HOOK()
#endif
/// Code to run that extends getFlagArgStr()'s functionality to custom types
#ifndef GET_FLAG_ARGS_STR_HOOK
  #define GET_FLAG_ARGS_STR_HOOK(type)
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
Difficulty parseDifficulty(int* argc, char*** argv, 
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


/// Use this to signify that the parsing was not successful
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
      bool ignored = false;
      if (*arg != '-') {
        
        for (; *arg; arg++) {
          switch (*arg) {
            #define X(type, longName, shortName, defaultValue, desc, parser)        \
              case shortName:                                                       \
                if (ignored)                                                        \
                  parser(argc, argv, ctx, &failed);                                 \
                else                                                                \
                  ctx->longName = parser(argc, argv, ctx, &failed);                 \
                if (failed)                                                         \
                  fprintf(stderr,                                                   \
                          "ERROR: flag '-%c' is missing a required argument: %s\n", \
                          shortName, getFlagArgsStr(#type));                        \
                break;

            LONG_FLAG_LIST()
            #undef X
            case '/':
              ignored = true;
              break;
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
        if (arg[0] == '/') {
          ignored = true;
          arg++;
        }

        #define X(type, longName, shortName, defaultValue, desc, parser)            \
          if (strcmp(arg, #longName) == 0) {                                        \
            if (ignored)                                                            \
              parser(argc, argv, ctx, &failed);                                     \
            else                                                                    \
              ctx->longName = parser(argc, argv, ctx, &failed);                     \
            if (failed)                                                             \
              fprintf(stderr,                                                       \
                      "ERROR: flag \"--%s\" is missing a required argument: %s\n",  \
                      #longName, getFlagArgsStr(#type));                            \
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
      printf("\t-%c, --%-10s %-20s\t%s (default: %s)\n",
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
               _unused FlagContext* ctx, _unused bool* failed) {
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
                  _unused FlagContext* ctx, _unused bool* failed) {
  PRELUDE();
  assert(failed);

  char* arg = popArg(argc, argv); 
  if (!arg)
    *failed = true;
  return arg;
}

Difficulty parseDifficulty(int* argc, char*** argv, 
                           _unused FlagContext* ctx, _unused bool* failed) {
  PRELUDE();
  assert(failed);

  char* arg = popArg(argc, argv); 
  if (!arg)
    *failed = true;

  if (strcmp(arg, "easy") == 0)
    return Easy;
  if (strcmp(arg, "normal") == 0)
    return Normal;
  if (strcmp(arg, "hard") == 0)
    return Hard;


  fprintf(stderr, 
          "ERROR: Invalid difficulty type \"%s\"\n", 
          arg);
  *failed = true;
  return Normal;
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
  if (strcmp(type, "Difficulty") == 0)
    return "<easy|normal|hard>";

  return "";
}

#endif
