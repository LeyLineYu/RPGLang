#include "io/io.h"
#include "logger/logger.h"
#include "error/error.h"
#include "frontend/frontend.h"
#include "frontend/lexer.h"
#include "frontend/preparser.h"
#include "frontend/parser.h"
#include "frontend/symtab.h"
#include <string.h>

Error frontend(TranslationUnit* trUnit, MappedFile inputFile) {
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
  // nodeDump(GRAPH_DUMP, trUnit->ast, "Parsed Tree");

  if ((err = symtabInit(trUnit, SYMTAB_BUCKET_SIZE, 
                        SYMTAB_LIST_CAPACITY, SYMTAB_HASH_FUNC))) {
    logln(FATAL, "Failed to init symtab\n");
    DEFER(err);
  }
  symtabInited = true;

  // hashTableDump(GRAPH_DUMP, &trUnit->symtab, "MANGLING");
  // nodeDump(GRAPH_DUMP, trUnit->ast, "After Symtab Init");

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
