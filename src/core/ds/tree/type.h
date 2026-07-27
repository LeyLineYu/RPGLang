#ifndef NODE_TYPE_H
#define NODE_TYPE_H

#include "error/error.h"
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#define NODE_TYPE_LIST()             \
  X(UNKNOWN_TYPE,    "UNKNOWN_TYPE") \
  X(OP_TYPE,         "OP")           \
  X(CTRL_TYPE,       "CTRL")         \
  X(NUM_TYPE,        "NUM")          \
  X(RAW_IDENT_TYPE,  "RAW_IDENT")    \
  X(SYMBOL_TYPE,     "SYMBOL")       \
  X(VAR_TYPE_TYPE,   "TYPE")

typedef enum {
  #define X(enm, ...) enm,
  NODE_TYPE_LIST()
  #undef X
} NodeType;

const char* getNodeTypeStr(NodeType type);
int getNodeType(const char* str, size_t n);

//NOTE:
//X(enum, "str")
#define VAR_TYPE_LIST() \
  X(TYPE_PRIM, "prim")  \
  X(TYPE_FRAC, "frac")  \
  X(TYPE_LOC,  "loc")   \
  X(TYPE_VOID, "void")

//NOTE:
//X(enum, "str")
#define CTRL_TYPE_LIST()         \
  X(CTRL_SEMIC,     ";")         \
  X(CTRL_ARG,       "arg")       \
  X(CTRL_ASG,       "=")         \
  X(CTRL_IF,        "if")        \
  X(CTRL_ELSE,      "else")      \
  X(CTRL_WHILE,     "while")     \
  X(CTRL_UNTIL,     "until")     \
  X(CTRL_DECL,      "decl")      \
  X(CTRL_PARAM,     "parameter") \
  X(CTRL_FUNC_DECL, "func_decl") \
  X(CTRL_FUNC_CALL, "func_call") \
  X(CTRL_SIGNATURE, "signature") \
  X(CTRL_RETURN,    "return")    \
  X(CTRL_CONTINUE,  "continue")  \
  X(CTRL_BREAK,     "break")

//TODO: priorities here are messed up; fix them
//NOTE:
//X(enum, "str", argc, prior)
#define OP_TYPE_LIST()       \
  X(OP_NOT,  "!",      1, 0) \
  X(OP_ADD,  "+",      2, 1) \
  X(OP_SUB,  "-",      2, 1) \
  X(OP_MUL,  "*",      2, 2) \
  X(OP_DIV,  "/",      2, 2) \
  X(OP_SHL,  "shl",    2, 3) \
  X(OP_SHR,  "shr",    2, 3) \
  X(OP_GRT,  "grt",    2, 4) \
  X(OP_LSR,  "lsr",    2, 4) \
  X(OP_EQ,   "==",     2, 5) \
  X(OP_NEQ,  "!=",     2, 5) \
  X(OP_AND,  "and",    2, 5) \
  X(OP_OR,   "or",     2, 5)

typedef enum {
  #define X(enm, ...) enm,
  OP_TYPE_LIST()
  #undef X
} OpType;

typedef struct {
  OpType type;
  const char* str;
  uint argCount;
  uint priority;
} OpTypeInfo;

const OpTypeInfo* parseOpType(OpType type);
int getOpType(const char* str, size_t n);
///Applies appropriate operation regarding a and b and returns the result.
///If the operation doesn't require a second parameter then leave b as whatever
int64_t applyOperation(OpType type, int64_t a, int64_t b, Error* status);

typedef enum {
  #define X(enm, ...) enm,
  CTRL_TYPE_LIST()
  #undef X
} CtrlType;

const char* getCtrlTypeStr(CtrlType type);
int getCtrlType(const char* str, size_t n);

typedef enum {
  #define X(enm, ...) enm,
  VAR_TYPE_LIST()
  #undef X
} VarType;

const char* getVarTypeStr(VarType type);
int getVarTypeType(const char* str, size_t n);

// Node utility macros
#define IS_TYPE(node, t) ((node) && (node)->data.type == t) 
#define IS_OP(node)        IS_TYPE(node, OP_TYPE)
#define IS_NUM(node)       IS_TYPE(node, NUM_TYPE)
#define IS_RAW_IDENT(node) IS_TYPE(node, RAW_IDENT_TYPE)
#define IS_CTRL(node)      IS_TYPE(node, CTRL_TYPE)
#define IS_VAR_TYPE(node)  IS_TYPE(node, VAR_TYPE_TYPE)
#define IS_SYMBOL(node)    IS_TYPE(node, SYMBOL_TYPE)
#define OF_VAR_TYPE(node, type) \
  (IS_VAR_TYPE((node)) && (node)->data.value.varType == (type))
#define OF_CTRL(node, ctrlType) \
  (IS_CTRL((node)) && (node)->data.value.ctrl == (ctrlType))
#define OF_OP(node, opType) \
  (IS_OP((node)) && (node)->data.value.op == (opType))
#define OF_NUM(node, i) \
  (IS_NUM((node)) && ((node)->data.value.num == (i)))

// Quick TreeNode and NodeUnit initializers
// inv means isInvalid
#define OP_UNIT_(i, inv) \
  (NodeUnit){.type = OP_TYPE,   .value = {.op = i}, .exceptionCount = (uint64_t)inv}
#define CTRL_UNIT_(i, inv) \
  (NodeUnit){.type = CTRL_TYPE, .value = {.ctrl = i}, .exceptionCount = (uint64_t)inv}
#define NUM_UNIT_(i, inv) \
  (NodeUnit){.type = NUM_TYPE,  .value = {.num = i}, .exceptionCount = (uint64_t)inv}
#define RAW_IDENT_UNIT_(name, len)  (NodeUnit){.type = RAW_IDENT_TYPE, .value = {.rawId = (StringView){name, len}}}
#define VAR_TYPE_UNIT_(i)  (NodeUnit){.type = VAR_TYPE_TYPE, .value = {.varType = i}}
#define SYMBOL_UNIT_(bucket, index)  (NodeUnit){.type = SYMBOL_TYPE, .value = {.sym = (SymbolIndex){.bucketIndex = bucket, .listIndex = index}}}
#define SYMBOL_OFFSETTED_UNIT_(arg, off)  (NodeUnit){.type = SYMBOL_TYPE, .value = {.symOff = (SymbolOffset){.isArg = arg, .offset = off}}}

#define NUM_(i, inv) nodeAlloc(NUM_UNIT_(i, inv))
#define RAW_IDENT_(name, len) nodeAlloc(RAW_IDENT_UNIT_(name, len))
#define SYMBOL_(bucket, index) nodeAlloc(SYMBOL_UNIT_(bucket, index))
#define SYMBOL_OFFSETTED_(isArg, offset) nodeAlloc(SYMBOL_OFFSETTED_UNIT_(isArg, offset))

#define PRIM_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_PRIM))
#define FRAC_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_FRAC))
#define LOC_()  nodeAlloc(VAR_TYPE_UNIT_(TYPE_LOC))
#define VOID_() nodeAlloc(VAR_TYPE_UNIT_(TYPE_VOID))

#define nodeAllocCtrl(ctrl, inv, l, r) \
  nodeAlloc(.data = CTRL_UNIT_(ctrl, inv), .left = l, .right = r)

#define SEMIC_(l) \
        nodeAllocCtrl(CTRL_SEMIC, false, l, NULL)
#define ARG_(l) \
        nodeAllocCtrl(CTRL_ARG, false, l, NULL)
#define ASG_(l, r, inv) \
        nodeAllocCtrl(CTRL_ASG, inv, l, r)
#define IF_(l, r, inv) \
        nodeAllocCtrl(CTRL_IF, inv, l, r)
#define ELSE_(r, inv) \
        nodeAllocCtrl(CTRL_ELSE, inv, r, NULL)
#define DECL_(l, r) \
        nodeAllocCtrl(CTRL_DECL, false, l, r)
#define PARAM_(l, r) \
        nodeAllocCtrl(CTRL_PARAM, false, l, r)
#define FUNC_DECL_(l, r) \
        nodeAllocCtrl(CTRL_FUNC_DECL, false, l, r)
#define SIGNATURE_(l, r) \
        nodeAllocCtrl(CTRL_SIGNATURE, false, l, r)
#define FUNC_CALL_(l, r) \
        nodeAllocCtrl(CTRL_FUNC_CALL, false, l, r)
#define RETURN_(l) \
        nodeAllocCtrl(CTRL_RETURN, false, l, NULL)
#define BREAK_() \
        nodeAllocCtrl(CTRL_BREAK, false, NULL, NULL)
#define CONTINUE_() \
        nodeAllocCtrl(CTRL_CONTINUE, false, NULL, NULL)

#define nodeAllocBinop(op, l, r, inv) \
  nodeAlloc(.data = OP_UNIT_(op, inv), .left = l, .right = r)
#define nodeAllocUnop(op, r, inv) \
  nodeAllocBinop(op, NULL, r, inv)

#define ADD_(l, r, inv) \
        nodeAllocBinop(OP_ADD, l, r, inv)
#define SUB_(l, r, inv) \
        nodeAllocBinop(OP_SUB, l, r, inv)
#define MUL_(l, r, inv) \
        nodeAllocBinop(OP_MUL, l, r, inv)
#define DIV_(l, r, inv) \
        nodeAllocBinop(OP_DIV, l, r, inv)
#define NOT_(r, inv) \
        nodeAllocUnop(OP_NOT, r, inv)
#define AND_(l, r, inv) \
        nodeAllocBinop(OP_AND, l, r, inv)
#define OR_(l, r, inv) \
        nodeAllocBinop(OP_OR, l, r, inv)
#define GRT_(l, r, inv) \
        nodeAllocBinop(OP_GRT, l, r, inv)
#define LSR_(l, r, inv) \
        nodeAllocBinop(OP_LSR, l, r, inv)
#define EQ_(l, r, inv) \
        nodeAllocBinop(OP_EQ, l, r, inv)
#define NEQ_(l, r, inv) \
        nodeAllocBinop(OP_NEQ, l, r, inv)
#define SHR_(l, r, inv) \
        nodeAllocBinop(OP_SHR, l, r, inv)
#define SHL_(l, r, inv) \
        nodeAllocBinop(OP_SHL, l, r, inv)
#define NEG_(r, inv) \
        nodeAllocBinop(OP_MUL, NUM_(-1, false), r, inv)
#define INV_(r, inv) \
        nodeAllocBinop(OP_DIV, NUM_(1, false), r, inv)
#define NEG_INV_(r, inv) \
        nodeAllocBinop(OP_DIV, NUM_(-1, false), r, inv)

#endif
