#include "backend/api/backend.h"
#include "backend/api/codegen.h"
#include "ds/tree/node.h"
#include "ds/tree/type.h"
#include "utils/utils.h"
#include <stdlib.h>

// TODO: rename "exceptions" into "saving throws" or smth like that

static Error mergeExceptionsCallback(TreeNode* node, uint level, void* data);

#define CMD_BUF_SZ 256
static const char* const ASM_FILEPATH        = "nasm.s";
static const char* const STDLIB_FILEPATH     = "stdlib.s";
static const char* const STDLIB_OBJ_FILEPATH = "stdlib.o";
static const char* const OBJ_FILEPATH        = "obj.o";

Error backend(const char* outputFilepath, TranslationUnit* trUnit,
              bool stopAtNasm, bool keepTempFiles) {
  if (!trUnit || !trUnit->ast ||
      !outputFilepath)
    return BadArgs;
  Error err = OK;
  if ((err = hashTableVerify(&trUnit->symtab)))
    return err;
  
  // nodeDump(GRAPH_DUMP, trUnit.ast, "<b2>AST</b2>");
  uint64_t* excPtr = NULL;
  nodeTraverse(trUnit->ast, 
               .prefix = mergeExceptionsCallback, 
               .prefixData = &excPtr);

  // TODO: use tmpnam or smth like that
  FILE* nasmFile = fopen(stopAtNasm
                         ? outputFilepath
                         : ASM_FILEPATH, "w");
  if (!nasmFile)
    return FailFileOpen;
  codegen(nasmFile, trUnit);
  fclose(nasmFile);
  nasmFile = NULL;

  if (stopAtNasm)
    return OK;

  char cmd[CMD_BUF_SZ] = {};
#define execf(str, ...)                    \
  snprintf(cmd, CMD_BUF_SZ,                \
           str __VA_OPT__(,) __VA_ARGS__); \
  if (system(cmd))                         \
    return Fail;
#define NASM_FLAGS "-f elf64 -wno-number-overflow"

  execf("nasm "NASM_FLAGS" \"%s\" -o \"%s\"", 
        ASM_FILEPATH, OBJ_FILEPATH);
  execf("nasm "NASM_FLAGS" \"%s\" -o \"%s\"", 
        STDLIB_FILEPATH, STDLIB_OBJ_FILEPATH);
  execf("ld \"%s\" \"%s\" -o \"%s\"", 
        OBJ_FILEPATH, STDLIB_OBJ_FILEPATH, outputFilepath);

#undef NASM_FLAGS
#undef execf

  if (!keepTempFiles) {
    if (remove(ASM_FILEPATH)) return Fail; 
    if (remove(OBJ_FILEPATH)) return Fail;
    if (remove(STDLIB_OBJ_FILEPATH)) return Fail;
  }
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
