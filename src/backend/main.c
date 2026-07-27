#include "ds/tree/node.h"
#include "flag/flag.h"
#include "io/io.h"
#include "utils/utils.h"
#include "backend/backend.h"

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  // bool htmlLogInited = false;
  bool mapFileInited = false;
  bool trUnitInited  = false;
  bool outFileInited = false;

  loggerInit(NULL, ERROR);
  loggerInited = true;

  Error err = OK;
  MappedFile mf = {};
  if ((err = mappedFileInit(&mf, input))) {
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
  // if (!GRAPH_DUMP) {
  //  exitValue = FailFileOpen; 
  //  goto exit;
  // }
  // htmlLogInited = true;

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
   logln(FATAL, "Failed to open \"%s\" for write\n", output);
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
  //   closeHtmlLogFile(GRAPH_DUMP);
  if (trUnitInited) {
    hashTableDestroy(&trUnit.symtab, false);
    nodeDestroy(trUnit.ast);
  }
  if (mapFileInited)
    mappedFileDestroy(&mf);
  if (outFileInited)
    fclose(outFile);
  return exitValue;
}
