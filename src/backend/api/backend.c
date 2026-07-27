#include "backend/api/backend.h"
#include "backend/api/codegen.h"
#include "ds/tree/node.h"
#include "ds/tree/type.h"
#include "utils/utils.h"

// TODO: rename "exceptions" into "saving throws" or smth like that

static Error mergeExceptionsCallback(TreeNode* node, uint level, void* data);

Error backend(FILE* outputFile, TranslationUnit* trUnit) {
  if (!trUnit || !trUnit->ast ||
      !outputFile)
    return BadArgs;
  Error err = OK;
  if ((err = hashTableVerify(&trUnit->symtab)))
    return err;
  
  // nodeDump(GRAPH_DUMP, trUnit.ast, "<b2>AST</b2>");
  uint64_t* excPtr = NULL;
  nodeTraverse(trUnit->ast, 
               .prefix = mergeExceptionsCallback, 
               .prefixData = &excPtr);

  codegen(outputFile, trUnit);

  return OK;
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
