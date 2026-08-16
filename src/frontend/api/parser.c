#include "frontend/api/parser.h"
#include "logger/logger.h"

typedef struct {
  Tokens* t;
  size_t  i;
} Parser;

static bool getFunctionDeclaration(Parser* p, TreeNode** result);
static bool getParameterList(Parser* p, TreeNode** result);
static bool getParameter(Parser* p, TreeNode** result);
static bool getStatement(Parser* p, TreeNode** result);
static bool getStatementBlock(Parser* p, TreeNode** result);
static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Parser* p, TreeNode** result);
static bool getIf(Parser* p, TreeNode** result);
#define getWhile(p, result) getConditionBlock(TOK_WHILE, CTRL_WHILE, p, result)
#define getUntil(p, result) getConditionBlock(TOK_UNTIL, CTRL_UNTIL, p, result)
static bool getVariableDeclaration(Parser* p, TreeNode** result);
static bool getAssignment(Parser* p, TreeNode** result);
static bool getAssignmentBody(Parser* p, TreeNode** result, bool* isInvalid);
static bool getReturn(Parser* p, TreeNode** result);
static bool getBreak(Parser* p, TreeNode** result);
static bool getContinue(Parser* p, TreeNode** result);
static bool getFunctionCall(Parser* p, TreeNode** result);
static bool getArgumentList(Parser* p, TreeNode** result);
static bool getExpression(Parser* p, TreeNode** result);
static bool getAnd(Parser* p, TreeNode** result);
static bool getEquality(Parser* p, TreeNode** result);
static bool getRelation(Parser* p, TreeNode** result);
static bool getShift(Parser* p, TreeNode** result);
static bool getAddition(Parser* p, TreeNode** result);
static bool getTerm(Parser* p, TreeNode** result);
static bool getUnary(Parser* p, TreeNode** result);
static bool getPrimary(Parser* p, TreeNode** result);
static bool getType(Parser* p, TreeNode** result);
static bool getNonVoidType(Parser* p, TreeNode** result);
static bool getIdentifier(Parser* p, TreeNode** result);
static bool getNumber(Parser* p, TreeNode** result);
static bool consumeToken(Parser* p, TokenType type);
static bool consumeClassifiedToken(Parser* p, TokenType type, bool* isInvalid);

// this function isn't finalized. 
// represents Grammar in grammar.txt, 
// which is currently FunctionDeclaration+, EOF
TreeNode* parse(Tokens* t) {
  if (dynArrVerify(t))
    return NULL;

  Parser p = (Parser){ .t = t, .i = 0 };
  TreeNode* firstFunc = NULL;
  if (!getFunctionDeclaration(&p, &firstFunc)) {
    logln(ERROR, "Program has no functions. Try writing \"prim main() {}\"");
    return NULL;
  }
  firstFunc = SEMIC_(firstFunc);

  TreeNode* nextFunc = NULL;
  for (TreeNode* curFunc = firstFunc;
       getFunctionDeclaration(&p, &nextFunc);) {
    nextFunc = SEMIC_(nextFunc);
    while (curFunc->right)
      curFunc = curFunc->right;

    curFunc->right = nextFunc;
    curFunc = nextFunc;
  }

  TokenType nextType = ((Token*)dynArrGet(t, p.i++))->type; 
  if (nextType != TOK_EOF) {
    logln(ERROR, "Expected EOF, got %s", getTokenTypeStr(nextType));
    nodeDestroy(firstFunc);
    return NULL;
  }
  nodeFixParents(firstFunc);
  return firstFunc;
}

#define PEEK() ((Token*)dynArrGet(p->t, p->i))
#define CHECK(T) (PEEK()->type == T)

#ifdef PARSER_DEBUG_INFO
  #define PRELUDE()                         \
    assert(p);                              \
    assert(p->t);                           \
    assert(result);                         \
    logln(DEBUG, "%s is at this token: %s", \
                 __PRETTY_FUNCTION__,       \
                 getTokenTypeStr(PEEK()->type));
#else
  #define PRELUDE()                         \
    assert(p);                              \
    assert(p->t);                           \
    assert(result);
#endif

static bool getFunctionDeclaration(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* type   = NULL;
  TreeNode* ident  = NULL;
  TreeNode* params = NULL;
  TreeNode* body   = NULL;
  if (getType(p, &type)                     &&
      getIdentifier(p, &ident)              &&
      consumeToken(p, TOK_LPAREN)           &&
      (getParameterList(p, &params) || 1)   && 
      consumeToken(p, TOK_RPAREN)           &&
      getStatement(p, &body)) {
    *result = FUNC_DECL_(SIGNATURE_(DECL_(type, ident), params), body);
    return true;
  }

  nodeDestroy(type);
  nodeDestroy(ident);
  nodeDestroy(params);
  nodeDestroy(body);
  return false;
}

static bool getParameterList(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getParameter(p, &first))
    return false;

  first = SEMIC_(first);
  for (TreeNode* last = first; 
       consumeToken(p, TOK_SEMIC);) {
    TreeNode* next = NULL;
    if (!getParameter(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    next = SEMIC_(next);
    last->right = next;
    last = next;
  }
  *result = first;
  return true;
}

static bool getParameter(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* type  = NULL;
  TreeNode* ident = NULL;
  if (getNonVoidType(p, &type) &&
      getIdentifier(p, &ident)) {
    *result = PARAM_(type, ident);
    return true;
  }

  nodeDestroy(type);
  nodeDestroy(ident);
  return false;
}

static bool getStatement(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_SEMIC)) {
    *result = SEMIC_(NULL);
    return true;
  }

  TreeNode* stmt = NULL;
  // statements that do not require a semicolon
  if (getStatementBlock(p, &stmt) ||
      getIf(p, &stmt)             ||
      getWhile(p, &stmt)          ||
      getUntil(p, &stmt)) {
    *result = stmt;
    return true;
  } 

  // statements that require a semicolon
  if (getVariableDeclaration(p, &stmt) ||
      getAssignment(p, &stmt)          ||
      getReturn(p, &stmt)              ||
      getContinue(p, &stmt)            ||
      getBreak(p, &stmt)               ||
      getExpression(p, &stmt)) {

    if (!consumeToken(p, TOK_SEMIC)) {
      nodeDestroy(stmt);
      return false;
    }

    *result = SEMIC_(stmt);
    return true;
  }

  return false;
}

static bool getStatementBlock(Parser* p, TreeNode** result) {
  PRELUDE();
  if (!consumeToken(p, TOK_LBRACE))
    return false;
  if (consumeToken(p, TOK_RBRACE)) {
    *result = SEMIC_(NULL);
    return true;
  }

  TreeNode* firstStmt = NULL;
  if (!getStatement(p, &firstStmt))
    return false;

  TreeNode* nextStmt = NULL;
  for (TreeNode* lastStmt = firstStmt;
       getStatement(p, &nextStmt);) {
    // if the statement is empty
    if (OF_CTRL(nextStmt, CTRL_SEMIC) && 
        !nextStmt->left) {
      nodeDestroy(nextStmt);
      continue;
    }

    while (lastStmt->right)
      lastStmt = lastStmt->right;

    lastStmt->right = nextStmt;
    lastStmt = nextStmt;
  }

  if (!consumeToken(p, TOK_RBRACE)) {
    nodeDestroy(firstStmt);
    return false;
  }

  *result = SEMIC_(firstStmt);
  return true;
}

static bool getConditionBlock(TokenType tokType, CtrlType ctrlType, 
                              Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  bool inv = false;
  if (consumeClassifiedToken(p, tokType, &inv)    &&
      consumeToken(p, TOK_LPAREN) &&
      getExpression(p, &lhs)      &&
      consumeToken(p, TOK_RPAREN) &&
      getStatement(p, &rhs)) {
    *result = nodeAllocCtrl(ctrlType, inv, lhs, rhs);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  return false;
}

static bool getIf(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* ifNode   = NULL;
  TreeNode* elseStmt = NULL;
  if (getConditionBlock(TOK_IF, CTRL_IF, p, &ifNode)) {
    bool inv = false;
    if (consumeClassifiedToken(p, TOK_ELSE, &inv)) {
      if (getStatement(p, &elseStmt)) {
        TreeNode* lastStmt = ifNode;
        while (lastStmt->right)
          lastStmt = lastStmt->right;

        lastStmt->right = ELSE_(elseStmt, inv);
      } else {
        nodeDestroy(ifNode);
        return false;
      }
    }

    *result = ifNode;
    return true;
  }

  nodeDestroy(ifNode);
  nodeDestroy(elseStmt);
  return false; 
}

static bool getVariableDeclaration(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* type = NULL;
  TreeNode* lhs = NULL;
  if (getNonVoidType(p, &type) &&
      getIdentifier(p, &lhs)) {
    TreeNode* rhs = NULL;
    bool inv = false;
    size_t oldI = p->i;
    if (getAssignmentBody(p, &rhs, &inv)) {
      *result = DECL_(type, ASG_(lhs, rhs, inv));
      return true;
    }
    p->i = oldI;
    nodeDestroy(rhs);
    *result = DECL_(type, lhs);
    return true;
  }

  nodeDestroy(lhs);
  return false;
}

static bool getAssignment(Parser* p, TreeNode** result) {
  PRELUDE();
  size_t oldI = p->i;
  TreeNode* lhs = NULL;
  TreeNode* rhs = NULL;
  bool inv = false;
  if (getIdentifier(p, &lhs) &&
      getAssignmentBody(p, &rhs, &inv)) {
    *result = ASG_(lhs, rhs, inv);
    return true;
  }

  nodeDestroy(lhs);
  nodeDestroy(rhs);
  p->i = oldI;
  return false;
}

static bool getAssignmentBody(Parser* p, TreeNode** result, bool* isInvalid) {
  PRELUDE();
  TreeNode* expr = NULL;
  if (consumeClassifiedToken(p, TOK_MIRROR, isInvalid) &&
      getExpression(p, &expr)) {
    *result = expr;
    return true;
  }

  nodeDestroy(expr);
  return false;
}

static bool getReturn(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_COMPLETE)) {
    TreeNode* expr = NULL;
    *result = getExpression(p, &expr) 
              ? RETURN_(expr)
              : RETURN_(NULL);
    
    return true;
  }

  return false;
}

static bool getContinue(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_ROLLBACK)) {
    *result = CONTINUE_();
    return true;
  }

  return false;
}

static bool getBreak(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_SKIP)) {
    *result = BREAK_(); 
    return true;
  }

  return false;
}

static bool getFunctionCall(Parser* p, TreeNode** result) {
  PRELUDE();
  size_t oldI = p->i;
  TreeNode* ident = NULL;
  TreeNode* args  = NULL;
  if (getIdentifier(p, &ident)          &&
      consumeToken(p, TOK_LPAREN)       &&
      (getArgumentList(p, &args) || 1)  && 
      consumeToken(p, TOK_RPAREN)) {
    *result = FUNC_CALL_(ident, args);
    return true;
  }

  nodeDestroy(ident);
  nodeDestroy(args);
  p->i = oldI;
  return false;
}

static bool getArgumentList(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getExpression(p, &first))
    return false;

  TreeNode* last = ARG_(first);
  while (consumeToken(p, TOK_SEMIC)) {
    TreeNode* next = NULL;
    if (!getExpression(p, &next)) {
      nodeDestroy(last);
      return false;
    }
    next = ARG_(next);
    next->right = last;
    last = next;
  }
  *result = last;
  return true;
}

static bool getExpression(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getAnd(p, &first))
    return false;
  bool inv = false;
  while (consumeClassifiedToken(p, TOK_OR, &inv)) {
    TreeNode* next = NULL;
    if (!getAnd(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = OR_(first, next, inv);
  }
  *result = first;
  return true;
}

static bool getAnd(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getEquality(p, &first))
    return false;
  bool inv = false;
  while (consumeClassifiedToken(p, TOK_AND, &inv)) {
    TreeNode* next = NULL;
    if (!getEquality(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = AND_(first, next, inv);
  }
  *result = first;
  return true;
}

static bool getEquality(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getRelation(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_WORTHY ||
       opTok->type == TOK_DUELLR;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getRelation(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_WORTHY
            ?  EQ_(first, next, opTok->isInvalidClass)
            : NEQ_(first, next, opTok->isInvalidClass);
  }
  *result = first;
  return true;
}

static bool getRelation(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getShift(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_DUELL ||
       opTok->type == TOK_DUELR;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getShift(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_DUELR
            ? LSR_(first, next, opTok->isInvalidClass)
            : GRT_(first, next, opTok->isInvalidClass);
  }
  *result = first;
  return true;
}

static bool getShift(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getAddition(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_PUSHL ||
       opTok->type == TOK_PUSHR;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getAddition(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_PUSHL
            ? SHL_(first, next, opTok->isInvalidClass)
            : SHR_(first, next, opTok->isInvalidClass);
  }
  *result = first;
  return true;
}

static bool getAddition(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getTerm(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_UNITE ||
       opTok->type == TOK_HIT;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getTerm(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_UNITE
            ? ADD_(first, next, opTok->isInvalidClass)
            : SUB_(first, next, opTok->isInvalidClass);
  }
  *result = first;
  return true;
}

static bool getTerm(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode* first = NULL;
  if (!getUnary(p, &first))
    return false;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_EMPOWER ||
       opTok->type == TOK_SHATTER;
       opTok = PEEK()) {
    p->i++;
    TreeNode* next = NULL;
    if (!getUnary(p, &next)) {
      nodeDestroy(first);
      return false;
    }
    first = opTok->type == TOK_EMPOWER
            ? MUL_(first, next, opTok->isInvalidClass)
            : DIV_(first, next, opTok->isInvalidClass);
  }
  *result = first;
  return true;
}

static bool getUnary(Parser* p, TreeNode** result) {
  PRELUDE();
  TreeNode*  first   = NULL;
  TreeNode** primary = NULL;
  for (Token* opTok = PEEK(); 
       opTok->type == TOK_SHADOW ||
       opTok->type == TOK_NOT;
       opTok = PEEK()) {
    p->i++;
    first = opTok->type == TOK_SHADOW
            ? NEG_(first, opTok->isInvalidClass)
            : NOT_(first, opTok->isInvalidClass);
    if (!primary)
      primary = &first->right;
  }
  if (!first)
    primary = &first;

  if (!getPrimary(p, primary)) {
    nodeDestroy(first);
    return false;
  }

  *result = first;
  return true;
}

static bool getPrimary(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_LPAREN)) {
    TreeNode* expr = NULL;
    if (getExpression(p, &expr) &&
        consumeToken(p, TOK_RPAREN)) {
      *result = expr;
      return true;
    }

    nodeDestroy(expr);
    return false;
  }

  TreeNode* prim = NULL;
  if (getNumber(p, &prim)       ||
      getFunctionCall(p, &prim) ||
      getIdentifier(p, &prim)) {
    *result = prim;
    return true;
  }

  nodeDestroy(prim);
  return false;
}

static bool consumeToken(Parser* p, TokenType type) {
  assert(p && p->t);
  if (CHECK(type)) {
    p->i++;
#ifdef PARSER_DEBUG_INFO
    logln(DEBUG, "consumed %s", getTokenTypeStr(type));
#endif
    return true;
  }
  return false;
}

static bool consumeClassifiedToken(Parser* p, TokenType type, bool* isInvalid) {
  assert(p && p->t && isInvalid);
  *isInvalid = PEEK()->isInvalidClass;
  return consumeToken(p, type);
}

static bool getIdentifier(Parser* p, TreeNode** result) {
  PRELUDE();
  if (CHECK(TOK_IDENTIFIER)) {
    *result = RAW_IDENT_(PEEK()->pos, PEEK()->len);
    p->i++;
    return true;
  }
  return false;
}

static bool getNumber(Parser* p, TreeNode** result) {
  PRELUDE();
  if (CHECK(TOK_NUM_LIT)) {
    *result = NUM_(PEEK()->value, PEEK()->isInvalidClass);
    p->i++;
    return true;
  }
  return false;
}

static bool getType(Parser* p, TreeNode** result) {
  PRELUDE();
  if (consumeToken(p, TOK_VOID)) {
    *result = VOID_();
    return true;
  }
  return getNonVoidType(p, result);
}

static bool getNonVoidType(Parser* p, TreeNode** result) {
  PRELUDE();
  switch (PEEK()->type) {
    case TOK_PRIM: *result = PRIM_(); p->i++; return true;
    //case TOK_FRAC: *result = FRAC_(); p->i++; return true;
    //case TOK_LOC:  *result = LOC_();  p->i++; return true;
    default: return false;
  }
}
