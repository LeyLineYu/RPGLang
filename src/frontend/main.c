#include "flag/flag.h"
#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/frontend.h"
#include <string.h>

int main(int argc, char* argv[]) {
  const char* inputFilepath  = NULL;
  const char* outputFilepath = NULL;
  parseArgs(&argc, &argv, &inputFilepath, &outputFilepath);

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

  FILE* outFile = fopen(outputFilepath, "w");
  if (!outFile) {
    logln(FATAL, "Failed to open \"%s\" for write", outputFilepath);
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
