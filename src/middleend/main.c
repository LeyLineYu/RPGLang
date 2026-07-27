#include "flag/flag.h"
#include "io/io.h"
#include "middleend/middleend.h"
#include "utils/utils.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
  const char* inputFilepath  = NULL;
  const char* outputFilepath = NULL;
  parseArgs(&argc, &argv, &inputFilepath, &outputFilepath);

  int  exitValue = 0;
  bool loggerInited  = false;
  // bool htmlLogInited = false;
  bool mapFileInited = false;
  bool trUnitInited  = false;

  loggerInit(NULL, ERROR);
  loggerInited = true;

  Error err = OK;
  MappedFile mf = {};
  if ((err = mappedFileInit(&mf, inputFilepath))) {
    logln(FATAL, "mappedFileInit returned %s\n", parseError(err)->str);
    DEFER(err);
  }
  mapFileInited = true;

  TranslationUnit trUnit = (TranslationUnit){};
  if ((err = translationUnitRead(&mf, &trUnit))) {
    logln(FATAL, "translationUnitRead returned %s\n", parseError(err)->str);
    DEFER(err);
  }
  trUnitInited = true;

  // GRAPH_DUMP = openHtmlLogFile("./.log/");
  // if (!GRAPH_DUMP)
  //   DEFER(FailFileOpen);
  // htmlLogInited = true;

  if ((err = middleend(&trUnit))) {
    logln(FATAL, "Middleend failed");
    DEFER(err);
  }

  FILE* outFile = fopen(outputFilepath, "w");
  if (!outFile) {
    logln(FATAL, "Failed to open \"%s\" for write\n", outputFilepath);
    DEFER(FailFileOpen);
  }
  translationUnitPrint(outFile, &trUnit);
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  // if (htmlLogInited)
  //   closeHtmlLogFile(GRAPH_DUMP);
  if (trUnitInited) {
    hashTableDestroy(&trUnit.symtab, false);
    nodeDestroy(trUnit.ast);
  }
  if (mapFileInited)
    mappedFileDestroy(&mf);
  return exitValue;
}
