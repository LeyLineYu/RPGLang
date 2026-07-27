#include "frontend/flag.h"
#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/api/frontend.h"
#include <string.h>

int main(int argc, char* argv[]) {
  FlagContext flagCtx = {0};
  flagContextInit(&flagCtx);
  parseArgs(&argc, &argv, &flagCtx);

  int  exitValue = 0;
  bool loggerInited    = false;
  // bool htmlLogInited   = false;
  bool inputFileMapped = false;
  bool trUnitInited    = false;
  
  loggerInit(NULL, ERROR);
  loggerInited = true;

  // GRAPH_DUMP = openHtmlLogFile("./.log/");
  // if (!GRAPH_DUMP)                      
  //   DEFER(FailFileOpen);                                  
  // htmlLogInited = true;

  Error err = OK;
  MappedFile inputFile = {};
  if ((err = mappedFileInit(&inputFile, flagCtx.input))) {
    logln(FATAL, "Mapping input file \"%s\" failed", flagCtx.input);
    DEFER(err);
  }
  inputFileMapped = true;

  TranslationUnit trUnit = {};
  if ((err = frontend(&trUnit, inputFile))) {
    logln(FATAL, "Frontend failed");
    DEFER(err);
  }
  trUnitInited = true;

  FILE* outFile = fopen(flagCtx.output, "w");
  if (!outFile) {
    logln(FATAL, "Failed to open \"%s\" for write", flagCtx.output);
    DEFER(FailFileOpen);
  }

  translationUnitPrint(outFile, &trUnit);
  fclose(outFile);

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
  return exitValue;
}
