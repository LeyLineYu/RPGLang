#include "ds/tree/node.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

TreeNode* nodeAlloc_(NodeAllocOpt opt) {
  TreeNode* node = (TreeNode*)calloc(1, sizeof(TreeNode));
  Error* status = opt.status;
  if (!node)
    RETURN_WITH_STATUS(FailMemoryAllocation, NULL);
  
  //так как NodeAllocOpt повторяет те же поля что и TreeNode, можем сделать так
  return memcpy(node, &opt, sizeof(TreeNode));
}

Error nodeTraverse_(TreeNode* node, NodeTraverseOpt opt) {
  uint oldLevel = opt.level;
  opt.level++;
  return (opt.prefix  && opt.prefix(node,  oldLevel, opt.prefixData)) ||
         (node        && nodeTraverse_(node->left,  opt))             ||
         (opt.infix   && opt.infix(node,   oldLevel, opt.infixData))  ||
         (node        && nodeTraverse_(node->right, opt))             ||
         (opt.postfix && opt.postfix(node, oldLevel, opt.postfixData));
}

TreeNode* nodeCopy(TreeNode* src, TreeNode* newParent, Error* status) {
  if (!src)
    RETURN_WITH_STATUS(BadArgs, NULL);

  Error returnedStatus = OK;
  TreeNode* copy = nodeAlloc((NodeUnit){src->data.type, src->data.value}, newParent,
                                   NULL, NULL, &returnedStatus);
  if (returnedStatus) {
    nodeDestroy(copy);
    RETURN_WITH_STATUS(returnedStatus, NULL);
  }

  if (src->left)
    copy->left = nodeCopy(src->left, copy, &returnedStatus);
  if (src->right)
    copy->right = nodeCopy(src->right, copy, &returnedStatus);

  if (returnedStatus) {
    nodeDestroy(copy);
    RETURN_WITH_STATUS(returnedStatus, NULL);
  }

  return copy;
}

void nodeFixParents(TreeNode* node) {
  if (!node)
    return;

  if (node->left) {
    node->left->parent  = node;
    nodeFixParents(node->left);
  }
  if (node->right) {
    node->right->parent = node;
    nodeFixParents(node->right);
  }
}

Error nodeChangeChild(TreeNode* parent, TreeNode* child, 
                      TreeNode* newChild, size_t* nodeCount) {
  TreeNode** childPath = NULL;
  if (!parent) {
    if (newChild)
      newChild->parent = parent;
    nodeDestroyC(child, nodeCount);
  } else if (*(childPath = &parent->left)  == child ||
             *(childPath = &parent->right) == child) {
    *childPath = newChild;
    if (newChild)
      newChild->parent = parent;
    nodeDestroyC(child, nodeCount);
  }

  return OK;
}

Error nodeDeleteC(TreeNode* node, size_t* nodeCount) {
  if (!node)
    return BadArgs;

  if (node->parent) {
    if (node->parent->left == node)
      node->parent->left = NULL;
    else if (node->parent->right == node)
      node->parent->right = NULL;
  }

  return nodeDestroyC(node, nodeCount);
}

Error nodeDestroyC(TreeNode* node, size_t* nodeCount) {
  if (!node)
    return BadArgs;

  nodeDestroyC(node->left, nodeCount);
  nodeDestroyC(node->right, nodeCount);
  node->left   = NULL;
  node->right  = NULL;
  node->parent = NULL;
  node->data   = (NodeUnit){};

  free(node);
  if (nodeCount)
    (*nodeCount)--;

  return OK;
}
