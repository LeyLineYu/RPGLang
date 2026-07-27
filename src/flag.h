/// Non-modular main's flag template initialization
/// Should only be used in the respective main.c

#define FLAG_CONTEXT_ADDITIONAL_FIELDS() \
  char* input;

// type, longName, shortName, defValue, desc, parser
#define LONG_FLAG_LIST()                                                                          \
  X(bool,  help,   'h', false,   "Display this message",                             parseBool)   \
  X(char*, output, 'o', "asm.s", "Place the output at the filepath set by <string>", parseString) \

#define REQUIRED_ARG_COUNT 1

#define USAGE(file)                                   \
  fprintf(file,                                       \
          "Usage: %s [OPTIONS] FILE\n"                \
          "Try \"%s --help\" for more information\n", \
          PROG_NAME, PROG_NAME);

#define USAGE_VERBOSE(file)                                                             \
  fprintf(file,                                                                         \
          "Usage: %s [OPTIONS] FILE\n"                                                  \
          "Compile FILE into nasm (given that FILE contains a valid RPGLang program)\n" \
          "Example: %s main.rpg -o output.asm\n",                                       \
          PROG_NAME, PROG_NAME);

#define DEFAULT_ARG_HANDLER()                                         \
  {                                                                   \
  if (ctx->input) {                                                   \
    fprintf(stderr, "ERROR: More than one input file is provided\n"); \
    FAILED();                                                         \
  }                                                                   \
  ctx->input = arg;                                                   \
  }
#define POST_PARSING_HOOK()                                \
  {                                                        \
  if (!ctx->input) {                                       \
    fprintf(stderr, "ERROR: No input file is provided\n"); \
    FAILED();                                              \
  }                                                        \
  }

#include "templates/flag.h"
