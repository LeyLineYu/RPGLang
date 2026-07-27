#include "backend/backend.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/frontend.h"
#include "middleend/middleend.h"

#define FLAG_CONTEXT_ADDITIONAL_FIELDS() \
  char* input;

// type, longName, shortName, defValue, desc, parser
#define LONG_FLAG_LIST() \
  X(bool, run, 'r', false, "Run the program", parseBool) \
  X(bool, help, 'h', false, "Display this message", parseBool) \
  X(char*, output, 'o', "blabaweif", "A very important string", parseString) \
  X(bool, verbose, 'v', false, "Yap", parseBool)

#define REQUIRED_ARG_COUNT 1

#define USAGE(file)                                 \
  fprintf(file,                                     \
          "Usage: %s [OPTIONS] FILE\n"              \
          "Try '%s --help' for more information\n", \
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

int main(int argc, char* argv[]) {
  FlagContext flagCtx = {0};
  flagContextInit(&flagCtx);
  parseArgs(&argc, &argv, &flagCtx);
  printf(".input = %s\n"
         ".verbose = %d\n"
         ".help = %d\n"
         ".run = %d\n"
         ".output = %s\n",
         flagCtx.input,
         flagCtx.verbose,
         flagCtx.help,
         flagCtx.run,
         flagCtx.output);

  return 0;

  const char* inputFilepath  = NULL;
  const char* outputFilepath = NULL;
  //parseArgs(&argc, &argv, &inputFilepath, &outputFilepath);

  int  exitValue = 0;
  bool loggerInited    = false;
  // bool htmlLogInited   = false;
  bool inputFileMapped = false;
  bool trUnitInited    = false;
  bool outFileInited   = false;
  
  loggerInit(NULL, ERROR);
  loggerInited = true;

  // GRAPH_DUMP = openHtmlLogFile("./.log/");
  // if (!GRAPH_DUMP)                      
  //   DEFER(FailFileOpen);                                  
  // htmlLogInited = true;

  Error err = OK;
  MappedFile inputFile = {};
  if ((err = mappedFileInit(&inputFile, inputFilepath))) {
    logln(FATAL, "Mapping input file \"%s\" failed", inputFilepath);
    DEFER(err);
  }
  inputFileMapped = true;

  TranslationUnit trUnit = {};
  if ((err = frontend(&trUnit, inputFile))) {
    logln(FATAL, "Frontend failed");
    DEFER(err);
  }
  trUnitInited = true;

  if ((err = middleend(&trUnit))) {
    logln(FATAL, "Frontend failed");
    DEFER(err);
  }

  FILE* outFile = fopen(outputFilepath, "w");
  if (!outFile) {
   logln(FATAL, "Failed to open \"%s\" for write\n", outputFilepath);
   DEFER(FailFileOpen);
  }
  outFileInited = true;

  if ((err = backend(outFile, &trUnit))) {
    logln(FATAL, "Backend failed");
    DEFER(err);
  }

exit:
  if (loggerInited)
    loggerCloseFile();
  // if (htmlLogInited)
  //    closeHtmlLogFile(GRAPH_DUMP);
  if (inputFileMapped)
    mappedFileDestroy(&inputFile);
  if (trUnitInited) {
    nodeDestroy(trUnit.ast);
    hashTableDestroy(&trUnit.symtab, false);
  }
  if (outFileInited)
    fclose(outFile);
  return exitValue;
}
