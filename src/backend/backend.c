#include "backend/codegen.h"
#include "ds/dump.h"
#include "ds/tree/node.h"
#include "ds/tree/type.h"
#include "io/io.h"
#include "utils/utils.h"

static Error mergeExceptionsCallback(TreeNode* node, uint level, void* data);

// TODO: factor this out alike frontend

int main(int argc, char* argv[]) {
  const char* input  = NULL;
  const char* output = NULL;
  parseArgs(&argc, &argv, &input, &output);

  int  exitValue = 0;
  bool loggerInited  = false;
  bool htmlLogInited = false;
  bool mapFileInited = false;
  bool trUnitInited  = false;
  loggerInit(NULL, ERROR);
  loggerInited = true;

  MappedFile mf = {};
  if ((exitValue = mappedFileInit(&mf, input))) {
    fprintf(stderr, "mappedFileInit returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  mapFileInited = true;

  TranslationUnit trUnit = (TranslationUnit){};
  if ((exitValue = translationUnitRead(&mf, &trUnit))) {
    fprintf(stderr, "translationUnitRead returned %s\n", parseError(exitValue)->str);
    goto exit;
  }
  trUnitInited = true;

  FILE* logFile = openHtmlLogFile("./.log/");
  if (!logFile) {
   exitValue = FailFileOpen; 
   goto exit;
  }
  htmlLogInited = true;

  nodeDump(logFile, trUnit.ast, "<b2>hello</b2>");
  // TODO: factor this out
  uint64_t* excPtr = NULL;
  nodeTraverse(trUnit.ast, 
               .prefix = mergeExceptionsCallback, 
               .prefixData = &excPtr);

  FILE* outFile = fopen(output, "w");
  if (!outFile) {
   fprintf(stderr, "Failed to open \"%s\" for write\n", output);
   exitValue = FailFileOpen;
   goto exit;
  }
  codegen(outFile, &trUnit);
  
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  if (htmlLogInited)
    closeHtmlLogFile(logFile);
  if (trUnitInited) {
    hashTableDestroy(&trUnit.symtab, false);
    nodeDestroy(trUnit.ast);
  }
  if (mapFileInited)
    mappedFileDestroy(&mf);
  return exitValue;
}

static Error mergeExceptionsCallback(TreeNode* node, 
                                     _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!node)
    return OK;

  uint64_t** exc = (uint64_t**)data;
  if (OF_CTRL(node, CTRL_SEMIC) ||
      OF_CTRL(node, CTRL_UNTIL) ||
      OF_CTRL(node, CTRL_WHILE) ||
      OF_CTRL(node, CTRL_IF)) {
    *exc = &node->data.exceptionCount;
    return OK;
  }

  if (!*exc)
    return OK;

  **exc += node->data.exceptionCount;
  node->data.exceptionCount = 0;
  return OK;
}
