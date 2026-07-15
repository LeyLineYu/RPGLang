#include "ds/dump.h"
#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/frontend.h"
#include "frontend/lexer.h"
#include "frontend/preparser.h"
#include "frontend/parser.h"
#include "frontend/symtab.h"
#include <string.h>

int main(int argc, char* argv[]) {
  const char* inputFilename  = NULL;
  const char* outputFilename = NULL;
  parseArgs(&argc, &argv, &inputFilename, &outputFilename);

  int  exitValue = 0;
  bool loggerInited    = false;
  // bool htmlLogInited   = false;
  bool inputFileMapped = false;
  bool trUnitInited    = false;
  
  loggerInit(NULL, ERROR);
  loggerInited = true;

   // FILE* graphDumpLog = openHtmlLogFile("./.log/");
   // if (!graphDumpLog) {                          
   //  exitValue = FailFileOpen;              
   //  goto exit;                           
   // }                                   
   // htmlLogInited = true;

  Error err = OK;
  MappedFile inputFile = {};
  if ((err = mappedFileInit(&inputFile, inputFilename))) {
    logln(FATAL, "Mapping input file \"%s\" failed", inputFilename);
    DEFER(err);
  }
  inputFileMapped = true;

  TranslationUnit trUnit = {};
  if ((err = frontend(&trUnit, inputFile, NULL/* graphDumpLog */))) {
    logln(FATAL, "Frontend failed");
    DEFER(err);
  }
  trUnitInited = true;

  FILE* outFile = fopen(outputFilename, "w");
  if (!outFile) {
    logln(FATAL, "Failed to open \"%s\" for write", outputFilename);
    DEFER(FailFileOpen);
  }

  translationUnitPrint(outFile, &trUnit);
  fclose(outFile);

exit:
  if (loggerInited)
    loggerCloseFile();
  // if (htmlLogInited)
  //    closeHtmlLogFile(graphDumpLog);
  if (inputFileMapped)
    mappedFileDestroy(&inputFile);
  if (trUnitInited) {
    nodeDestroy(trUnit.ast);
    hashTableDestroy(&trUnit.symtab, false);
  }
  return exitValue;
}


Error frontend(TranslationUnit* trUnit, MappedFile inputFile, 
               _unused FILE* graphDumpFile) {
  if (!trUnit || 
      !inputFile.size || !inputFile.data)
    return BadArgs;

  Error exitValue = 0;
  bool lexerInited  = false;
  bool astInited    = false;
  bool symtabInited = false;

  Error err = OK;
  Lexer lexer = (Lexer){};
  if ((err = lexerInit(&lexer, inputFile, LEXER_INIT_CAP))) {
    logln(FATAL, "lexerInit returned %s", parseError(err)->str);
    DEFER(err);
  }
  lexerInited = true;

  if ((err = lexerAnalyze(&lexer))) {
    logln(FATAL, "lexerAnalyze returned %s", parseError(err)->str);
    DEFER(err);
  }

  // lexerPrintTokens(stdout, &lexer);

  _unused bool valid = preparse(&lexer.tokens, &err);

  if (err) {
    logln(FATAL, "Preparser Failed\n");
    DEFER(err);
  }
#ifndef HARD_DIFFICULTY
  if (!valid) {
    logln(FATAL, "Invalid class usage detected, no further compilation is done\n");
    DEFER(Fail);
  }
#endif

  // printf("After Preparsing ------------\n");
  // lexerPrintTokens(stdout, &lexer);

  trUnit->ast = parse(&lexer.tokens);
  if (!trUnit->ast) {
    logln(FATAL, "Failed to parse token stream\n");
    DEFER(Fail);
  }
  astInited = true;
   // if (graphDumpFile)
   //   nodeDump(graphDumpFile, trUnit->ast, "Parsed Tree");

  if ((err = symtabInit(trUnit, SYMTAB_BUCKET_SIZE, 
                        SYMTAB_LIST_CAPACITY, SYMTAB_HASH_FUNC))) {
    logln(FATAL, "Failed to init symtab\n");
    DEFER(err);
  }
  symtabInited = true;

   // if (graphDumpFile) {
   //   hashTableDump(graphDumpFile, &trUnit->symtab, "MANGLING");
   //   nodeDump(graphDumpFile, trUnit->ast, "After Symtab Init");
   // }

  if (!symtabCheckCalls(trUnit, &err)) {
    fprintf(stderr, "Invalid function call detected, no further compilation is done\n");
    DEFER(Fail);
  }
  if (err) {
    fprintf(stderr, "Failed to check function calls\n");
    DEFER(err);
  }
 
exit:
  if (lexerInited)
    lexerDestroy(&lexer, false);
  if (exitValue) {
    if (astInited) {
      nodeDestroy(trUnit->ast);
      trUnit->ast = NULL;
    }
    if (symtabInited)
      hashTableDestroy(&trUnit->symtab, false);
  }
  return exitValue;
}
