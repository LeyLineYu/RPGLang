#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include "ds/list/list.h"
#include "ds/tree/type.h"
#include "error/error.h"
#include "utils/utils.h"

typedef struct {
  size_t    bucketIndex;
  ListIndex listIndex;
} SymbolIndex;

typedef struct {
  bool    isArg;
  size_t offset;
} SymbolOffset;

typedef union {
  VarType varType;
  OpType op;
  CtrlType ctrl;
  StringView rawId;
  SymbolIndex sym;
  SymbolOffset symOff;
  int64_t num;
} NodeValue;

typedef struct {
  NodeType  type;
  NodeValue value;
  uint64_t  exceptionCount;
} NodeUnit;

#define TREE_NODE_FIELDS    \
  NodeUnit  data;           \
  struct TreeNode_* parent; \
  struct TreeNode_* left;   \
  struct TreeNode_* right

typedef struct TreeNode_ {
  TREE_NODE_FIELDS;
} TreeNode;

typedef struct {
  TREE_NODE_FIELDS;
  Error* status;
} NodeAllocOpt;

#undef TREE_NODE_FIELDS

TreeNode* nodeAlloc_(NodeAllocOpt options);
//other fields will be 0-initialized which is what we want
#define nodeAlloc(...) \
  nodeAlloc_((NodeAllocOpt){__VA_ARGS__})

// nodeDeleteC - delete node (and its children), with counter 
// being changed if it isn't NULL. Also removes the node from the children of its parents
Error  nodeDeleteC(TreeNode* node, size_t* nodeCount);
Error nodeDestroyC(TreeNode* node, size_t* nodeCount);
#define nodeDelete(node) nodeDeleteC(node, NULL);
#define nodeDestroy(node) nodeDestroyC(node, NULL);

typedef Error (*callback_f)(TreeNode* node, uint level, void* data);

typedef struct {
  callback_f prefix;
  callback_f infix;
  callback_f postfix;
  void* prefixData;
  void* infixData;
  void* postfixData;
  uint level;
} NodeTraverseOpt;

/// Universal Traverse - stops if any callback_f return non-zero
Error nodeTraverse_(TreeNode* node, NodeTraverseOpt opt);

//again, 0-initialized fields are helping us here
#define nodeTraverse(node, ...) \
  nodeTraverse_(node, (NodeTraverseOpt){ __VA_ARGS__ })

//Takes parent, seaches for child as its child, replaces that child with newChild and frees child
//Note: if your newChild and child are in a relationship then you should make sure they aren't by
//breaking the bound before calling the function
Error nodeChangeChild(TreeNode* parent, TreeNode* child, TreeNode* newChild,
                      size_t* nodeCount);

///Note: doesnt copy the parent field but instead assigns newParent as copy's parent
TreeNode*  nodeCopy(TreeNode* srcNode, TreeNode* newParent, Error* status);
void nodeFixParents(TreeNode* node);

#endif
