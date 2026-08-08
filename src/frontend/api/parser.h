#ifndef PARSER_H
#define PARSER_H

#include "ds/tree/node.h"
#include "frontend/api/lexer.h"

TreeNode* parse(Tokens* tokens); 

#endif
