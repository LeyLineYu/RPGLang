#include "frontend/api/symtab.h"
#include "ds/hashtable/entry.h"
#include "ds/tree/type.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  StringView name;
  size_t offset;
  bool isArg;
} ConvertRawVariableCallbackData;

static StringView mangleName(StringView name, Error* status);
static Error symtabAddStdlib(HashTable* symtab);
static Error checkCallsCallback(TreeNode* node, uint level, void* data);
static Error resolveFunctionVariables(TreeNode* node, uint64_t* argcPtr, uint64_t* varcPtr);
static Error resolveLocalVariablesCallback(TreeNode* node, uint level, void* data);
static Error populateSymtabCallback(TreeNode* node, uint level, void* data);
static Error convertRawIdentifiersCallback(TreeNode* node, uint level, void* data);
static Error convertRawVariableCallback(TreeNode* node, uint level, void* data);

Error symtabInit(TranslationUnit* trUnit, size_t bucketCount, 
                 size_t initialListCapacity, hash_f hashFunc) {
  if (!trUnit ||
      !trUnit->ast)
    return BadArgs;

  Error err = OK;
  if ((err = hashTableInit(&trUnit->symtab, bucketCount, 
                           initialListCapacity, sizeof(Symbol),
                           freeSymbol, printSymbol,
                           cmpSymbol, hashFunc)))
    return err;

  symtabAddStdlib(&trUnit->symtab);
  if (nodeTraverse(trUnit->ast, 
                   .postfix = populateSymtabCallback, 
                   .postfixData = &trUnit->symtab)) {
    fprintf(stderr, "Multiple declarations of the same symbol detected, "
                    "no further compilation is done\n");
    return Fail;
  }
  if (nodeTraverse(trUnit->ast, 
                    .postfix = convertRawIdentifiersCallback,
                    .postfixData = &trUnit->symtab)) {
    fprintf(stderr, "Unknown symbol being used, no further compilation is done\n");
    return Fail;
  }
  
  return OK;
}

bool symtabCheckCalls(TranslationUnit* trUnit, Error* status) {
  Error err = OK;
  if ((err = hashTableVerify(&trUnit->symtab)))
    RETURN_WITH_STATUS(err, false);
  if (!trUnit ||
      !trUnit->ast)
    RETURN_WITH_STATUS(BadArgs, false);

  return !nodeTraverse(trUnit->ast,
                       .postfix = checkCallsCallback,
                       .postfixData = &trUnit->symtab);
}

// NOTE: name, argc, hasReturnValue
#define STDLIB_FUNC_LIST() \
  X("out",    1, false)    \
  X("exit",   1, false)    \
  X("rout",   2, false)    \
  X("in",     0, true)     \
  X("random", 0, false)

static Error symtabAddStdlib(HashTable* symtab) {
  Error err = OK;
  if ((err = hashTableVerify(symtab)))
    return err;
  
  Symbol sym = (Symbol){.external = true};
#define X(name, argCount, ret)                  \
  sym.argc = argCount;                          \
  sym.mangledName = mangleName(SV(name), &err); \
  sym.hasReturnValue = ret;                     \
  if (err)                                      \
    return err;                                 \
  hashTablePut(symtab, SV(name), &sym);

  STDLIB_FUNC_LIST()

#undef X

  return OK;
}

static Error checkCallsCallback(TreeNode* node,
                                _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_FUNC_CALL))
    return OK;

  Error err = OK;
  HashTable* ht = (HashTable*)data;
  TreeNode* funcIdNode = node->left;
  if (!IS_SYMBOL(funcIdNode))
    return OK;
  SymbolIndex symIdx = funcIdNode->data.value.sym;
  Entry* entry = (Entry*)listGetValue(ht->buckets + symIdx.bucketIndex, 
                                      symIdx.listIndex, &err);
  if (err)
    return err;
  Symbol* sym = (Symbol*)listGetValue(&ht->values, entry->value, &err);
  if (err)
    return err;
  uint64_t argc = 0;
  TreeNode* argNode = node->right;
  while (argNode) {
    argc++;
    argNode = argNode->right;
  }
  if (argc != sym->argc) {
    //TODO: better diagnostics when call is invalid
    fprintf(stderr, 
            "[ERROR] function %.*s expects %lu arguments, but %lu were provided\n",
            (int)entry->key.size, entry->key.data, sym->argc, argc);
    return Fail;
  } 
  if (!sym->hasReturnValue &&
      !OF_CTRL(node->parent, CTRL_SEMIC)) {
    fprintf(stderr, 
            "[ERROR] void function %.*s used in a statement requiring a return value\n",
            (int)entry->key.size, entry->key.data);
    return Fail;
  }
  return OK;
}

static Error populateSymtabCallback(TreeNode* node, 
                                    _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_FUNC_DECL))
    return OK;

  Error err = OK;
  HashTable* ht = (HashTable*)data;
  TreeNode* funcIdNode = node->left->left->right;
  StringView funcName = funcIdNode->data.value.rawId;
  Symbol sym = (Symbol){};
  if (hashTableGet(ht, funcName, &sym, &err)) {
    fprintf(stderr, 
            "[ERROR]: Function %.*s has multiple definitions\n",
            (int)funcName.size, funcName.data);
    return Fail;
  }
  if (err)
    return err;

  uint64_t argc = 0, varc = 0;
  if ((err = resolveFunctionVariables(node, &argc, &varc)))
    return err;

  sym = (Symbol){
    .mangledName = mangleName(funcName, &err),
    .argc = argc,
    .varc = varc,
    .external = false,
    .hasReturnValue = !OF_VAR_TYPE(node->left->left->left, TYPE_VOID),
  };
  if (err)
    return err;

  size_t bucketIndex  = 0;
  ListIndex listIndex = 0;
  hashTablePutExt(ht, funcName, &sym, &bucketIndex, &listIndex);
  nodeChangeChild(funcIdNode->parent, funcIdNode, 
                  SYMBOL_(bucketIndex, listIndex), NULL);
  return OK;
}

static Error resolveFunctionVariables(TreeNode* node, uint64_t* argcPtr, uint64_t* varcPtr) {
  if (!OF_CTRL(node, CTRL_FUNC_DECL) ||
      !argcPtr || !varcPtr)
    return BadArgs;

  Error err = OK;
  uint64_t argc = 0;
  TreeNode* funcBody = node->right;
  TreeNode* paramNode = node->left->right;
  while (paramNode) {
    ConvertRawVariableCallbackData d = (ConvertRawVariableCallbackData) {
      .name = paramNode->left->right->data.value.rawId,
      .isArg = true,
      .offset = argc,
    };
    nodeChangeChild(paramNode->left->right->parent, paramNode->left->right,
                    SYMBOL_OFFSETTED_(true, argc), NULL);
    nodeTraverse(funcBody, 
                 .postfix = convertRawVariableCallback,
                 .postfixData = &d);
    if (err)
      return err;
    argc++;
    paramNode = paramNode->right;
  }

  uint64_t varc = 0;
  if ((err = nodeTraverse(funcBody, 
                          .postfix = resolveLocalVariablesCallback,
                          .postfixData = &varc)))
    return err;

  *argcPtr = argc;
  *varcPtr = varc;
  return OK;
}

static Error resolveLocalVariablesCallback(TreeNode* node, 
                                           _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!OF_CTRL(node, CTRL_DECL))
    return OK;

  TreeNode* varNameNode = !OF_CTRL(node->right, CTRL_ASG) 
                          ? node->right
                          : node->right->left;
  assert(IS_RAW_IDENT(varNameNode));
  Error err = OK;
  uint64_t* varc = (uint64_t*)data;
  StringView* name = &varNameNode->data.value.rawId;

  ConvertRawVariableCallbackData d = (ConvertRawVariableCallbackData) {
    .name = *name,
    .isArg = false,
    .offset = *varc,
  };
  nodeChangeChild(varNameNode->parent, varNameNode,
                  SYMBOL_OFFSETTED_(false, *varc), NULL);
  if ((err = nodeTraverse(node->parent,
                          .postfix = convertRawVariableCallback,
                          .postfixData = &d)))
    return err;
  (*varc)++;

  return OK;
}

static Error convertRawIdentifiersCallback(TreeNode* node, 
                                           _unused uint level, void* data) {
  if (!data)
    return BadArgs;

  if (IS_RAW_IDENT(node) &&
      !OF_CTRL(node->parent, CTRL_FUNC_CALL)) {
    StringView* name = &node->data.value.rawId;
    fprintf(stderr, 
            "[ERROR] Symbol named \"%.*s\" is undeclared\n",
            (int)name->size, name->data);   
    return Fail;
  }
  if (!OF_CTRL(node, CTRL_FUNC_CALL))
    return OK;

  TreeNode* funcNameNode = node->left;
  Error err = OK;
  HashTable* ht = (HashTable*)data;
  StringView* name = &funcNameNode->data.value.rawId;
  Symbol* sym = NULL;
  size_t bucketIndex  = 0;
  ListIndex listIndex = 0;
  if (!hashTableGetExt(ht, *name, &sym, &bucketIndex, &listIndex, &err)) {
    fprintf(stderr, 
            "[ERROR] Function named \"%.*s\" is undeclared\n",
            (int)name->size, name->data);
    return Fail;
  }
  
  nodeChangeChild(funcNameNode->parent, funcNameNode, 
                  SYMBOL_(bucketIndex, listIndex), NULL);
  if (err)
    return err;
  return OK;
}

static const char ESCAPE_CHAR = '_';
static const char HEX_ALPHABET[] = "0123456789ABCDEF";
static const size_t HEX_LEN = sizeof(HEX_ALPHABET) - 1;

static StringView mangleName(StringView name, Error* status) {
  if (!name.data)
    RETURN_WITH_STATUS(BadArgs, name);

  size_t mangledLen = 0;
  for (size_t i = 0; i < name.size; i++) {
    if (name.data[i] == ESCAPE_CHAR) {
      mangledLen += 2;
      continue;
    }
    if (i == 0 && !isalpha(name.data[i]))
      mangledLen += 1;
    if (!isalnum(name.data[i])) {
      mangledLen += 3;
      continue;
    }
    mangledLen++;
  }
  
  char* mangledName = (char*)calloc(mangledLen + 1, sizeof(char));
  if (!mangledName)
    RETURN_WITH_STATUS(FailMemoryAllocation, name);

  char* dest = mangledName;
  for (size_t i = 0; i < name.size; i++) {
    unsigned char c = (unsigned char)name.data[i];
    if (c == ESCAPE_CHAR) {
      *dest++ = ESCAPE_CHAR;
      *dest++ = ESCAPE_CHAR;
      continue;
    }
    if (i == 0 && !isalpha(c))
      *dest++ = ESCAPE_CHAR;
    if (!isalnum(c)) {
      *dest++ = ESCAPE_CHAR;
      *dest++ = HEX_ALPHABET[c / HEX_LEN];
      *dest++ = HEX_ALPHABET[c % HEX_LEN];
      continue;
    }
    *dest++ = name.data[i];
  }

  StringView result = (StringView){
    .data = mangledName,
    .size = mangledLen,
  };
  return result;
}

static Error convertRawVariableCallback(TreeNode* node, 
                                        _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!IS_RAW_IDENT(node) ||
      OF_CTRL(node->parent, CTRL_FUNC_CALL))
    return OK;

  Error err = OK;
  ConvertRawVariableCallbackData* d = (ConvertRawVariableCallbackData*)data;
  StringView varName = node->data.value.rawId;
  if (varName.size == d->name.size &&
      strncmp(varName.data, d->name.data, d->name.size) == 0) {

    if (OF_CTRL(node->parent, CTRL_DECL) ||
        (OF_CTRL(node->parent, CTRL_ASG) &&
         node == node->parent->left &&
         OF_CTRL(node->parent->parent, CTRL_DECL))) {
      fprintf(stderr, 
              "[ERROR] Variable \"%.*s\" has multiple declarations\n",
              (int)varName.size, varName.data);
      return Fail;     
    }
    
    nodeChangeChild(node->parent, node,
                    SYMBOL_OFFSETTED_(d->isArg, d->offset), NULL);
    if (err)
      return err;
  }

  return OK;
}
