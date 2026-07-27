#include "middleend/api/middleend.h"
#include "middleend/api/optimization.h"

Error middleend(TranslationUnit* trUnit) {
  if (!trUnit || !trUnit->ast)
    return BadArgs;
  Error err = OK;
  if ((err = hashTableVerify(&trUnit->symtab)))
    return err;

  // nodeDump(GRAPH_DUMP, trUnit->ast, "<b2>Before optimization</b2>");
  err = nodeOptimize(&trUnit->ast);
  // nodeDump(GRAPH_DUMP, trUnit->ast, "<b2>After optimization</b2>");
  return err;
}
