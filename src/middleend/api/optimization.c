#include "middleend/api/optimization.h"
#include "ds/tree/root.h"
#include <assert.h>

static int64_t nodeOptimizeConstants(TreeNode* node, size_t* nodeCount, Error* status);
static Error   nodeOptimizeNeutral(TreeNode** node, size_t* nodeCount);
static Error   countExceptionsCallback(TreeNode* node, 
                                       uint level, void* data);

Error nodeOptimize(TreeNode** node) {
  if (!node || !*node)
    return BadArgs;

  Error err = OK;
  TreeRoot* root = attachRoot(*node, &err);
  if (err)
    return err;
  size_t prevNodeCount = 0;
  do {
    prevNodeCount = root->nodeCount;
    nodeOptimizeConstants(*node, &root->nodeCount, NULL);
    nodeOptimizeNeutral(node, &root->nodeCount);
    nodeFixParents(*node);
  } while (prevNodeCount != root->nodeCount);

  detachRoot(root, &err);
  if (err)
    return err;
  return OK;
}

static int64_t nodeOptimizeConstants(TreeNode* node, size_t* nodeCount, Error* status) {
  if (!node)
    RETURN_WITH_STATUS(BadArgs, 0);
  if (!IS_OP(node)) {
    nodeOptimizeConstants(node->left, nodeCount, status);
    nodeOptimizeConstants(node->right, nodeCount, status);
    return 0;
  }

  OpType opType = node->data.value.op;
  const OpTypeInfo* i = parseOpType(opType);
  assert(i);
  switch (i->argCount) {
    case 1: {
      Error err = OK;
      int64_t rightVal = IS_NUM(node->right)
                         ? node->right->data.value.num
                         : nodeOptimizeConstants(node->right, nodeCount, &err);
      if (err)
        RETURN_WITH_STATUS(Fail, 0);

      int64_t result = applyOperation(opType, rightVal, 0, &err);
      if (err)
        RETURN_WITH_STATUS(err, 0);
      nodeTraverse(node->right, 
                   .infix = countExceptionsCallback, 
                   .infixData = &node->data.exceptionCount);
      nodeDeleteC(node->right, nodeCount);
      node->data.type = NUM_TYPE;
      node->data.value.num = result;
      return result;
    }
    case 2: {
      Error err = OK;
      int64_t leftVal  = IS_NUM(node->left)
                         ? node->left->data.value.num
                         : nodeOptimizeConstants(node->left, nodeCount, &err);
      int64_t rightVal = IS_NUM(node->right)
                         ? node->right->data.value.num
                         : nodeOptimizeConstants(node->right, nodeCount, &err);
      if (err)
        RETURN_WITH_STATUS(Fail, 0);

      int64_t result = applyOperation(opType, leftVal, rightVal, &err);
      if (err)
        RETURN_WITH_STATUS(err, 0);
      nodeTraverse(node->left, 
                   .infix = countExceptionsCallback, 
                   .infixData = &node->data.exceptionCount);
      nodeTraverse(node->right, 
                   .infix = countExceptionsCallback, 
                   .infixData = &node->data.exceptionCount);
      nodeDeleteC(node->left,  nodeCount);
      nodeDeleteC(node->right, nodeCount);
      node->data.type = NUM_TYPE;
      node->data.value.num = result;
      return result;
    }
    default:
      assert(0 && "unreacheable optimizeConstants");
      RETURN_WITH_STATUS(Fail, 0);
  }
}

#define REPLACE_WITH(newNode, otherNode)                        \
  {                                                             \
  newNode->data.exceptionCount += (*node)->data.exceptionCount; \
  nodeTraverse(otherNode,                                       \
               .infix = countExceptionsCallback,                \
               .infixData = &newNode->data.exceptionCount);     \
  TreeNode* newChild = newNode;                                 \
  newNode = NULL;                                               \
  TreeNode* parent = (*node)->parent;                           \
  nodeChangeChild(parent, *node, newChild, nodeCount);          \
  *node = newChild;                                             \
  }                                                             \

#define REDUCE_TO_NUM(nodeValue)                            \
  {                                                         \
  nodeTraverse((*node)->left,                               \
               .infix = countExceptionsCallback,            \
               .infixData = &(*node)->data.exceptionCount); \
  nodeTraverse((*node)->right,                              \
               .infix = countExceptionsCallback,            \
               .infixData = &(*node)->data.exceptionCount); \
  nodeDeleteC((*node)->left , nodeCount);                   \
  nodeDeleteC((*node)->right, nodeCount);                   \
  (*node)->data.type      = NUM_TYPE;                       \
  (*node)->data.value.num = nodeValue;                      \
  }

static Error nodeOptimizeNeutral(TreeNode** node, size_t* nodeCount) {
  if (!node ||
      !*node)
    return BadArgs;

  nodeOptimizeNeutral(&(*node)->left, nodeCount);
  nodeOptimizeNeutral(&(*node)->right, nodeCount);
  if (!IS_OP(*node))
    return OK;

  switch ((*node)->data.value.op) {
    case OP_MUL: {
      if (OF_NUM((*node)->left, 0) ||
          OF_NUM((*node)->right, 0)) {
        REDUCE_TO_NUM(0);
      } else if (OF_NUM((*node)->left, 1)) {
        REPLACE_WITH((*node)->right, (*node)->left);
      } else if (OF_NUM((*node)->right, 1)) {
        REPLACE_WITH((*node)->left, (*node)->right);
      }
      break;
    }
    case OP_DIV: {
      if (OF_NUM((*node)->left, 0)) {
        REDUCE_TO_NUM(0);
      } else if (OF_NUM((*node)->right, 1)) {
        REPLACE_WITH((*node)->left, (*node)->right);
      }
      break;
    }
    case OP_ADD: {
      if (OF_NUM((*node)->left, 0)) {
        REPLACE_WITH((*node)->right, (*node)->left);
      } else if (OF_NUM((*node)->right, 0)) {
        REPLACE_WITH((*node)->left, (*node)->right);
      }
      break;
    }
    case OP_SUB: {
      if (OF_NUM((*node)->right, 0)) {
        REPLACE_WITH((*node)->left, (*node)->right);
      }
      break;
    }
    default:
      break;
  }

  return OK;
}

#undef REPLACE_WITH
#undef REDUCE_TO_NUM

static Error countExceptionsCallback(TreeNode* node, 
                                     _unused uint level, void* data) {
  if (!data)
    return BadArgs;
  if (!node)
    return OK;
  uint64_t* exc = (uint64_t*)data;
  *exc += node->data.exceptionCount;
  return OK;
}
