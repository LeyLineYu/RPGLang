#include "ds/tree/node.h"
#include "backend/flag.h"
#include "io/io.h"
#include "utils/utils.h"
#include "backend/api/backend.h"

int main(int argc, char* argv[]) {
  FlagContext flagCtx = {0};
  flagContextInit(&flagCtx);
  parseArgs(&argc, &argv, &flagCtx);

  int  exitValue = 0;
  bool loggerInited  = false;
  // bool htmlLogInited = false;
  bool mapFileInited = false;
  bool trUnitInited  = false;

  loggerInit(NULL, ERROR);
  loggerInited = true;

  Error err = OK;
  MappedFile mf = {};
  if ((err = mappedFileInit(&mf, flagCtx.input))) {
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

  if ((err = backend(flagCtx.output, &trUnit, flagCtx.nasm, flagCtx.temp))) {
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
  return exitValue;
}
