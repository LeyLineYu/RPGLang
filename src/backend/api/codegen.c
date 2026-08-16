#include "backend/api/codegen.h"
#include "ds/hashtable/entry.h"
#include "logger/logger.h"
#include <assert.h>
#include <stdarg.h>

typedef struct {
  FILE* sink;
  HashTable* symtab;
  TreeNode* curFuncDecl;
  uint64_t labelCount;
  uint64_t depth;
} Context;

// each register is 8 bytes
static const size_t REG_SIZE = 8;
static const char* const ARG_REGS[] = {
  "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};
static const size_t ARG_REGS_SIZE = sizer(ARG_REGS);

static void codegenRec(Context* ctx, TreeNode* ast, 
                       uint64_t conditionLabel, uint64_t endLabel);
static inline void handleConditionLabels(Context* ctx, TreeNode* ast, 
                                         uint64_t* conditionLabel);
static inline void push(Context* ctx, TreeNode* ast);
static inline void op(Context* ctx, TreeNode* ast);
static inline void not(Context* ctx, TreeNode* ast);
static inline void or(Context* ctx, TreeNode* ast);
static inline void and(Context* ctx, TreeNode* ast);
static inline void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void handleBranches(Context* ctx, TreeNode* ast,
                                  uint64_t conditionLabel, uint64_t endLabel,
                                  uint64_t* newLabel);
static void handleIfBranches(Context* ctx, TreeNode* ast, 
                                    uint64_t label, uint64_t* newLabel);
static void handleLoopBranches(Context* ctx, TreeNode* ast,
                               uint64_t conditionLabel, uint64_t endLabel,
                               uint64_t* newLabel, CtrlType type,
                               const char* cmpStr);
static inline void call(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static inline void funcDecl(Context* ctx, TreeNode* ast);
static inline void asg(Context* ctx, TreeNode* ast);
static inline void raiseExceptions(Context* ctx, TreeNode* ast);
static void cmp(Context* ctx, TreeNode* ast, const char* cmpStr);
static void clearStack(Context* ctx, TreeNode* ast, uint64_t oldDepth);
static void gen_(FILE* sink, const char* commentary, 
                 const char* fmt, ...) _format(printf, 3, 4);

#define sinkFile sink
#define gen(commentary, fmt, ...) \
  gen_(sinkFile, "\n; -- " commentary " --\n", fmt __VA_OPT__(,) __VA_ARGS__)
#define genn(fmt, ...) \
  gen_(sinkFile, "", fmt __VA_OPT__(,) __VA_ARGS__)
#ifdef BACKEND_DEBUG_INFO
#define com(commentary) fputs("\n; -- " commentary " --\n", sinkFile)
#else
#define com(commentary)
#endif

// TODO: backend for: 
// CTRL: CTRL_CONTINUE, CTRL_BREAK
// TYPE: frac and loc

void codegen(FILE* sink, TranslationUnit* trUnit) {
  if (!sink || 
      !trUnit ||
      !trUnit->ast ||
      hashTableVerify(&trUnit->symtab))
    return;

  gen("HEADER",
      "global _start\n");
  com("EXTERNS");
  List* lst = &trUnit->symtab.values;
  for (ListIndex i = listGetHead(lst, NULL); i; i = lst->next[i]) {
    Symbol* s = (Symbol*)listGetValue(lst, i, NULL);
    if (s->external)
      genn("extern %.*s\n",
           (int)s->mangledName.size, s->mangledName.data);
  }
  gen("TEXT",
      "section .text\n"
      "_start:\n"
      "\t\tcall main\n"
      "\t\tpush rax\n");
  gen("EXIT",
      "\t\tmov rax, 0x3c ; syscall exit\n"
      "\t\tpop rdi\n"
      "\t\tsyscall\n");

  Context ctx = (Context){
    .sink = sink,
    .symtab = &trUnit->symtab,
    .curFuncDecl = NULL,
    .labelCount = 0,
    .depth = 0,
  };
  codegenRec(&ctx, trUnit->ast, 0, 0);
}

#undef sinkFile
#define sinkFile ctx->sink

static void codegenRec(Context* ctx, TreeNode* ast, 
                       uint64_t conditionLabel, uint64_t endLabel) {
  if (!ctx || 
      !ctx->sink ||
      !ast)
    return;

  if (OF_CTRL(ast, CTRL_FUNC_DECL))
      ctx->curFuncDecl = ast;

  if (OF_CTRL(ast, CTRL_FUNC_CALL)) {
    assert(ctx->curFuncDecl);
    SymbolIndex index = ctx->curFuncDecl->left->left->right->data.value.sym;
    Entry* e = (Entry*)listGetValue(ctx->symtab->buckets + index.bucketIndex, 
                                    index.listIndex, NULL);
    assert(e);
    Symbol* sym = (Symbol*)listGetValue(&ctx->symtab->values, e->value, NULL);
    assert(sym);
    if (sym->argc) {
      com("PRECALL SAVED REGS");
    }
    for (size_t i = 0; i < ARG_REGS_SIZE && i < sym->argc; i++) {
      genn("\t\tpush %s\n",
           ARG_REGS[i]);
      ctx->depth++;
    }
  }

  uint64_t oldDepth = ctx->depth;
  handleConditionLabels(ctx, ast, &conditionLabel);

  codegenRec(ctx, ast->left, 0, 0);

  uint64_t rightEndLabel = 0;
  handleBranches(ctx, ast, conditionLabel, endLabel, &rightEndLabel);

  // TODO: factor this out
  if (OF_CTRL(ast, CTRL_SEMIC) ||
      OF_CTRL(ast, CTRL_UNTIL) ||
      OF_CTRL(ast, CTRL_WHILE) ||
      OF_CTRL(ast, CTRL_IF)) {
    if (OF_CTRL(ast, CTRL_SEMIC))
      clearStack(ctx, ast, oldDepth);
    raiseExceptions(ctx, ast);
  }

  codegenRec(ctx, ast->right, conditionLabel, rightEndLabel);


  if (IS_NUM(ast)) {
    push(ctx, ast);    
    return;
  }

  // TODO: cleanup this monstrosity
  if (IS_SYMBOL(ast) && 
      !(OF_CTRL(ast->parent, CTRL_ASG) && ast == ast->parent->left) &&
      !OF_CTRL(ast->parent, CTRL_FUNC_CALL) &&
      !OF_CTRL(ast->parent, CTRL_DECL) &&
      !OF_CTRL(ast->parent, CTRL_PARAM)) {
    SymbolOffset s = ast->data.value.symOff;
    if (s.isArg) {
      if (s.offset < ARG_REGS_SIZE) {
        gen("PUSH REG ARGUMENT",
            "\t\tpush %s\n",
            ARG_REGS[s.offset]);
      } else {
        gen("PUSH STACK ARGUMENT",
            "\t\tmov rax, [rbp + %lu]\n"
            "\t\tpush rax\n",
            ((s.offset - (ARG_REGS_SIZE - 1)) + 1) * REG_SIZE);
      }
    } else {
      gen("PUSH STACK VARIABLE",
          "\t\tmov rax, [rbp - %lu]\n"
          "\t\tpush rax\n",
          ((s.offset + 1) * REG_SIZE));
    }

    ctx->depth++;
    return;
  }
  
  if (IS_OP(ast)) {
    op(ctx, ast); 
    return;
  }

  if (IS_CTRL(ast)) {
    ctrl(ctx, ast, oldDepth);
    return;
  }
}

#define PRELUDE()    \
  assert(ctx);       \
  assert(ctx->sink); \
  assert(ast);

static void push(Context* ctx, TreeNode* ast) {
  PRELUDE();
  gen("PUSH",
      "\t\tpush %ld\n", 
      ast->data.value.num);
  ctx->depth++;
}

static void op(Context* ctx, TreeNode* ast) {
  PRELUDE();
  if (ast->left) {
    genn("\t\tpop r10\n");
    ctx->depth--;
  }
  if (ast->right) {
    genn("\t\tpop rax\n");
    ctx->depth--;
  }
  switch (ast->data.value.op) {
    case OP_ADD:
      gen("ADD",
          "\t\tadd rax, r10\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SUB:
      gen("SUB",
          "\t\tsub rax, r10\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_MUL:
      gen("MUL",
          "\t\timul rax, r10\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_DIV:
      gen("DIV",
          "\t\tpush rdx\n"
          "\t\tcqo\n"
          "\t\tidiv r10\n"
          "\t\tpop rdx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SHL:
      gen("SHL",
          "\t\tpush rcx\n"
          "\t\tmov cl, r10b\n"
          "\t\tshl rax, cl\n"
          "\t\tpop rcx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_SHR:
      gen("SHR",
          "\t\tpush rcx\n"
          "\t\tmov cl, r10b\n"
          "\t\tshr rax, cl\n"
          "\t\tpop rcx\n"
          "\t\tpush rax\n");
      ctx->depth++;
      break;
    case OP_NOT:
      not(ctx, ast);
      break;
    case OP_AND:
      and(ctx, ast);
      break;
    case OP_OR:
      or(ctx, ast); 
      break;
#ifdef CONDITIONAL_MOVES
    case OP_GRT:
      com("GRT");
      cmp(ctx, ast, "cmovg");
      break;
    case OP_LSR:
      com("LSR");
      cmp(ctx, ast, "cmovl");
      break;    
    case OP_EQ:
      com("EQ");
      cmp(ctx, ast, "cmove");
      break;    
    case OP_NEQ:
      com("NEQ");
      cmp(ctx, ast, "cmovne");
      break;
#else
    case OP_GRT:
      com("GRT");
      cmp(ctx, ast, "jg");
      break;
    case OP_LSR:
      com("LSR");
      cmp(ctx, ast, "jl");
      break;
    case OP_EQ:
      com("EQ");
      cmp(ctx, ast, "je");
      break;
    case OP_NEQ:
      com("NEQ");
      cmp(ctx, ast, "jne");
      break;
#endif
    default: break;
  }
}

static void ctrl(Context* ctx, TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();
  switch (ast->data.value.ctrl) {
    case CTRL_FUNC_CALL:
      call(ctx, ast, oldDepth);
      break;
    case CTRL_RETURN:
      if (ast->left) {
        com("TYPED RETURN");
      } else {
        com("VOID RETURN");
      }
      if (ast->left) {
        genn("\t\tpop rax\n");
        ctx->depth--;
      }
      genn("\t\tmov rsp, rbp\n"
           "\t\tpop rbp\n"
           "\t\tret\n");         
      break;
    case CTRL_FUNC_DECL:
      gen("EXTRA RETURN",
          "\t\tmov rsp, rbp\n"
          "\t\tpop rbp\n"
          "\t\tret\n");
      break;
    case CTRL_SIGNATURE:
      funcDecl(ctx, ast);
      break;
    case CTRL_ASG:
      asg(ctx, ast);
      break;
    default: break;
  }
}

static void handleConditionLabels(Context* ctx, TreeNode* ast, uint64_t* conditionLabel) {
  PRELUDE();
  assert(conditionLabel);

  if (OF_CTRL(ast, CTRL_WHILE) ||
      OF_CTRL(ast, CTRL_UNTIL)) {
    *conditionLabel = ctx->labelCount++;
    gen("LOOP",
        ".L%zu:\n",
        *conditionLabel);
  }
}

static void handleBranches(Context* ctx, TreeNode* ast,
                           uint64_t conditionLabel, uint64_t endLabel,
                           uint64_t* newLabel) {
  handleIfBranches(ctx, ast, endLabel, newLabel); 
  handleLoopBranches(ctx, ast, 
                     conditionLabel, 
                     endLabel, newLabel,
                     CTRL_WHILE,
                     "jz");
  handleLoopBranches(ctx, ast, 
                     conditionLabel, 
                     endLabel, newLabel,
                     CTRL_UNTIL,
                     "jnz");
}

static void handleLoopBranches(Context* ctx, TreeNode* ast,
                               uint64_t conditionLabel, uint64_t endLabel,
                               uint64_t* newLabel, CtrlType type,
                               const char* cmpStr) {
  PRELUDE();
  assert(newLabel);
  if (OF_CTRL(ast, type)) {
    *newLabel = ctx->labelCount++;
    genn("\t\tpop rax\n"
         "\t\ttest rax, rax\n"
         "\t\t%s .L%zu\n",
         cmpStr, *newLabel);
    ctx->depth--;
    return;
  }

  if (OF_CTRL(ast->parent, type) &&
      ast->parent->right == ast) {
    genn("\t\tjmp .L%zu\n", 
         conditionLabel);
    genn(".L%zu:\n", 
         endLabel);
    return;
  }
}

static void handleIfBranches(Context* ctx, TreeNode* ast, 
                             uint64_t label, uint64_t* newLabel) {
  PRELUDE();
  assert(newLabel);

  if (OF_CTRL(ast, CTRL_IF)) {
    *newLabel = ctx->labelCount++;
    gen("IF",
        "\t\tpop rax\n"
        "\t\ttest rax, rax\n"
        "\t\tjz .L%zu\n",
        *newLabel);
    ctx->depth--;
    return;
  }

  if (OF_CTRL(ast, CTRL_ELSE)) {
    genn(".L%zu:\n", label);
    return;
  }

  if (OF_CTRL(ast->parent, CTRL_IF) &&
      ast->parent->right == ast) {
    if (OF_CTRL(ast->right, CTRL_ELSE)) {
      *newLabel = ctx->labelCount++;
      genn("\t\tjmp .L%zu\n", *newLabel);
    }
    genn(".L%zu:\n", label);
    return;
  }
}

static void clearStack(Context* ctx, _unused TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();
  if (ctx->depth != oldDepth) {
      gen("CLEAR STACK",
          "\t\tadd rsp, %zu\n",
          (ctx->depth - oldDepth) * REG_SIZE);
      ctx->depth = oldDepth;
  }
}

static void raiseExceptions(Context* ctx, TreeNode* ast) {
  PRELUDE();
  if (!ast->data.exceptionCount)
    return;
  com("EXCEPTION");
  for (size_t i = 0; i < ast->data.exceptionCount; i++)
    genn("\t\tcall random\n");
}

static void call(Context* ctx, TreeNode* ast, uint64_t oldDepth) {
  PRELUDE();

  com("CALL");
  size_t i = 0;
  for (TreeNode* arg = ast->right; 
       arg && i < ARG_REGS_SIZE; arg = arg->right) {
    genn("\t\tpop %s\n",
         ARG_REGS[i]);
    ctx->depth--;
    i++;
  }

  SymbolIndex id = ast->left->data.value.sym;
  Entry* e = (Entry*)listGetValue(ctx->symtab->buckets + id.bucketIndex, 
                                  id.listIndex, NULL);
  assert(e);
  Symbol* sym = (Symbol*)listGetValue(&ctx->symtab->values, e->value, NULL);
  assert(sym);
  genn("\t\tcall %.*s\n",
       (int)sym->mangledName.size, sym->mangledName.data);
  clearStack(ctx, ast, oldDepth); 

  assert(ctx->curFuncDecl);
  id = ctx->curFuncDecl->left->left->right->data.value.sym;
  e = (Entry*)listGetValue(ctx->symtab->buckets + id.bucketIndex, 
                           id.listIndex, NULL);
  assert(e);
  Symbol* curFuncDeclSym = (Symbol*)listGetValue(&ctx->symtab->values, e->value, NULL);
  assert(curFuncDeclSym);
  
  if (curFuncDeclSym->argc) {
    com("POP CALL SAVED REGS");
  }
  for (size_t k = MIN(6, curFuncDeclSym->argc) - 1; k < ARG_REGS_SIZE; k--) {
    genn("\t\tpop %s\n",
        ARG_REGS[k]);
    ctx->depth--;
  }

  if (sym->hasReturnValue) {
    genn("\t\tpush rax\n");
    ctx->depth++;
  }
}

static void asg(Context* ctx, TreeNode* ast) {
  PRELUDE();

  SymbolOffset s = ast->left->data.value.symOff;
  com("ASSIGNMENT");
  if (s.isArg && s.offset < ARG_REGS_SIZE) {
    genn("\t\tpop %s\n",
         ARG_REGS[s.offset]);
  } else if (s.isArg) {
    genn("\t\tpop rax\n"
         "\t\tmov qword[rbp + %lu], rax\n",
         ((s.offset - (ARG_REGS_SIZE - 1)) + 1) * REG_SIZE);
  } else {
    genn("\t\tpop rax\n"
         "\t\tmov qword[rbp - %lu], rax\n",
         ((s.offset + 1) * REG_SIZE));
  }
  ctx->depth--;
}

static void funcDecl(Context* ctx, TreeNode* ast) {
  PRELUDE();

  SymbolIndex id = ast->left->right->data.value.sym;
  Entry* e = (Entry*)listGetValue(ctx->symtab->buckets + id.bucketIndex, 
      id.listIndex, NULL);
  assert(e);
  Symbol* sym = (Symbol*)listGetValue(&ctx->symtab->values, e->value, NULL);
  assert(sym);
  gen("FUNC DECL",
      "%.*s:\n"
      "\t\tpush rbp\n"
      "\t\tmov rbp, rsp\n"
      "\t\tsub rsp, %lu\n",
      (int)sym->mangledName.size, sym->mangledName.data,
      sym->varc * REG_SIZE);
}

static void cmp(Context* ctx, _unused TreeNode* ast, const char* cmpStr) {
  PRELUDE();
  assert(cmpStr);
#ifdef CONDITIONAL_MOVES
  genn("\t\tmov r11, rax\n"
       "\t\txor eax, eax\n"
       "\t\tcmp r11, r10\n"
       "\t\tmov r11, 1\n"
       "\t\t%s rax, r11\n"
       "\t\tpush rax\n",
       cmpStr);
#else
  uint64_t pushZeroLabel     = ctx->labelCount++;
  uint64_t skipPushZeroLabel = ctx->labelCount++;
  genn("\t\tcmp rax, r10\n"
       "\t\t%s .push_one%zu\n"
       //"\t\t.push_zero%zu:\n"
       "\t\tpush 0\n"
       "\t\tjmp .skip_push_one%zu\n"
       ".push_one%zu:\n"
       "\t\tpush 1\n"
       ".skip_push_one%zu:\n",
       cmpStr,
       pushZeroLabel, skipPushZeroLabel,
       pushZeroLabel, skipPushZeroLabel);
#endif
  ctx->depth++;
}

static void not(Context* ctx, _unused TreeNode* ast) {
  PRELUDE();
#ifdef CONDITIONAL_MOVES
  gen("LOGICAL NOT",
      "\t\tmov r10, rax\n"
      "\t\txor eax, eax\n"
      "\t\ttest r10, r10\n"
      "\t\tmov r10, 1\n"
      "\t\tcmovz rax, r10\n"
      "\t\tpush rax\n");
#else
  uint64_t pushZeroLabel     = ctx->labelCount++;
  uint64_t skipPushZeroLabel = ctx->labelCount++;
  gen("LOGICAL NOT",
      "\t\ttest rax, rax\n"
      "\t\tjz .push_one%zu\n"
      "\t\tpush 0\n"
      "\t\tjmp .skip_push_one%zu\n"
      ".push_one%zu:\n"
      "\t\tpush 1\n"
      ".skip_push_one%zu:\n",
      pushZeroLabel, skipPushZeroLabel,
      pushZeroLabel, skipPushZeroLabel);    
#endif
  ctx->depth++;
}

static void and(Context* ctx, _unused TreeNode* ast) {
  PRELUDE();
  uint64_t falseLabel     = ctx->labelCount++;
  uint64_t skipFalseLabel = ctx->labelCount++;
  gen("LOGICAL AND",
      "\t\ttest rax, rax\n"
      "\t\tjz .false%zu\n"
      "\t\ttest r10, r10\n"
      "\t\tjz .false%zu\n"
      "\t\tpush 1\n"
      "\t\tjmp .skip_false%zu\n"
      ".false%zu:\n"
      "\t\tpush 0\n"
      ".skip_false%zu:\n",
      falseLabel,     falseLabel,
      skipFalseLabel, falseLabel,
      skipFalseLabel);    
  ctx->depth++;
}

static void or(Context* ctx, _unused TreeNode* ast) {
  PRELUDE();
  uint64_t trueLabel     = ctx->labelCount++;
  uint64_t skipTrueLabel = ctx->labelCount++;
  gen("LOGICAL OR",
      "\t\ttest rax, rax\n"
      "\t\tjnz .true%zu\n"
      "\t\ttest r10, r10\n"
      "\t\tjnz .true%zu\n"
      "\t\tpush 0\n"
      "\t\tjmp .skip_true%zu\n"
      ".true%zu:\n"
      "\t\tpush 1\n"
      ".skip_true%zu:\n",
      trueLabel,     trueLabel,
      skipTrueLabel, trueLabel,
      skipTrueLabel);    
  ctx->depth++;
}

#undef sinkFile
#undef gen
#undef genn
#undef com
#undef PRELUDE

static void gen_(FILE* sink, _unused const char* commentary, 
                 const char* fmt, ...) {
  assert(sink);
  assert(commentary);
  assert(fmt);

  #ifdef BACKEND_DEBUG_INFO
  fputs(commentary, sink);
  #endif

  va_list args = {};
  va_start(args, fmt);
  vfprintf(sink, fmt, args);
  va_end(args);
}
