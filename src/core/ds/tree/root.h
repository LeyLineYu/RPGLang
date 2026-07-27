#ifndef ROOT_H
#define ROOT_H

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "ds/tree/node.h"

typedef struct {
  size_t nodeCount;
  TreeNode* rootNode;
} TreeRoot;

TreeRoot* attachRoot(TreeNode* node, Error* status);
TreeRoot* attachRootC(TreeNode* node, size_t nodeCount, Error* status);
TreeNode* detachRoot(TreeRoot* root, Error* status);
Error rootDestroy(TreeRoot* tree);

#endif
