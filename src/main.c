#include "logger/logger.h"
#include "error/error.h"
#include "frontend/api/frontend.h"
#include "middleend/api/middleend.h"
#include "backend/api/backend.h"
#include "flag.h"

int main(int argc, char* argv[]) {
  FlagContext flagCtx = {0};
  flagContextInit(&flagCtx);
  parseArgs(&argc, &argv, &flagCtx);

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

  if ((err = middleend(&trUnit))) {
    logln(FATAL, "Frontend failed");
    DEFER(err);
  }

  FILE* outFile = fopen(flagCtx.output, "w");
  if (!outFile) {
   logln(FATAL, "Failed to open \"%s\" for write\n", flagCtx.output);
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
