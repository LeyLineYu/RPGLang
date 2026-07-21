#include "middleend/middleend.h"
#include "middleend/optimization.h"

Error middleend(TranslationUnit* trUnit) {
  // nodeDump(GRAPH_DUMP, trUnit.ast, "<b2>Before optimization</b2>");
  Error err = nodeOptimize(&trUnit->ast);
  // nodeDump(GRAPH_DUMP, trUnit.ast, "<b2>After optimization</b2>");
  return err;
}
