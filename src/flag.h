/// Non-modular main's flag template initialization
/// Should only be used in the respective main.c

#define FLAG_CONTEXT_ADDITIONAL_FIELDS() \
  char* input;

// type, longName, shortName, defValue, desc, parser
#define LONG_FLAG_LIST()                                                            \
  X(bool,       help,       'h', false,   "Display this message",       parseBool)  \
  X(bool,       nasm,       's', false,   "Compile and don't assemble", parseBool)  \
  X(bool,       temp,       't', false,   "Keep temporary files",       parseBool)  \
  X(bool,       optimize,   'O', true,    "Apply optimizations",        parseBool)  \
  X(char*,      output,     'o', "a.out", "Place the output at the "                \
                                          "filepath set by <string>", parseString)  \
  X(Difficulty, difficulty, 'D', Normal,  "Set difficulty mode for the compiler:\n" \
                                          "\t\t\t\teasy - Do not check for the "    \
                                          "correctness of class usage and do "      \
                                          "not produce saving throws "              \
                                          "for the class misusage\n"                \
                                          "\t\t\t\tnormal - Check for the "         \
                                          "correctness and stop the "               \
                                          "compilation upon found misusage\n"       \
                                          "\t\t\t\thard - Do as in 'normal' "       \
                                          "difficulty, but do not stop the "        \
                                          "compilation, letting saving throws be "  \
                                          "generated instead",        parseDifficulty)

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
