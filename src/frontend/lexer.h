#ifndef LEXER_H
#define LEXER_H

#include "ds/dynarr/dynarr.h"
#include "utils/utils.h"
#include <stdint.h>
 
// TODO: Implement keywords: 
//"encrypt" "freeze" "scry" "delayed" "prepare" "equip" "conjure" "slice" "slam" "purify" "train" "leech" "sacrifice" "roots" "natage" "natgrowth" "symbiosis" "blighten" "penance" "enlighten"

#define KEYWORD_LIST()           \
  X(TOK_NOTE, "note")            \
  X(TOK_MAGE, "#mage")           \
  X(TOK_WARRIOR, "#warrior")     \
  X(TOK_PRIEST, "#priest")       \
  X(TOK_WARLOCK, "#warlock")     \
  X(TOK_EMPOWER, "empower")      \
  X(TOK_MIRROR, "mirror")        \
  X(TOK_HIT, "hit")              \
  X(TOK_DUELL, "duel!<")         \
  X(TOK_DUELR, "duel!>")         \
  X(TOK_DUELLR, "duel!<>")       \
  X(TOK_WORTHY, "worthy")        \
  X(TOK_PUSHL, "<push")          \
  X(TOK_PUSHR, "push>")          \
  X(TOK_SHADOW, "shadow")        \
  X(TOK_UNTIL, "until")          \
  X(TOK_WHILE, "while")          \
  X(TOK_IF, "if")                \
  X(TOK_ELSE, "else")            \
  X(TOK_UNITE, "unite")          \
  X(TOK_AND, "and")              \
  X(TOK_NOT, "not")              \
  X(TOK_SHATTER, "shatter")      \
  X(TOK_OR, "or")                \
  X(TOK_COMPLETE, "complete")    \
  X(TOK_ROLLBACK, "rollback")    \
  X(TOK_SKIP, "skip")            \
  X(TOK_PRIM, "prim")            \
  X(TOK_VOID, "void")
//  X(TOK_FRAC, "frac")
//  X(TOK_LOC,  "loc")

#define KEYWORD_ALIAS_LIST() \
  X(TOK_PRIM, "primordial")
  //X(TOK_FRAC, "fractured")
  //X(TOK_LOC,  "location")

#define TOKEN_TYPE_LIST()          \
  X(TOK_EOF,        "EOF")         \
  X(TOK_IDENTIFIER, "IDENTIFIER")  \
  X(TOK_SEMIC,      "';'")         \
  X(TOK_LPAREN,     "'('")         \
  X(TOK_RPAREN,     "')'")         \
  X(TOK_LBRACE,     "'{'")         \
  X(TOK_RBRACE,     "'}'")         \
  X(TOK_NUM_LIT,    "NUM_LITERAL") \
  KEYWORD_LIST()

typedef enum {
  #define X(enm, ...) enm,
  TOKEN_TYPE_LIST()
  #undef X
} TokenType;

extern const size_t TOKEN_TYPES_SIZE; 

const char* getTokenTypeStr(TokenType type);

// TODO: add const qualifiers to char* here
typedef struct {
  TokenType type;
  bool isInvalidClass;
  int64_t value;
  size_t lineN;
  char* lineStart;
  char* pos;
  size_t len;
} Token;

typedef DynamicArray Tokens;

typedef struct {
  Tokens tokens;
  MappedFile mf;
  size_t lineN;
  char* lineStart;
  size_t pos;
} Lexer;

Error  lexerInit(Lexer* lexer, MappedFile file, size_t initCap);
Lexer* lexerAlloc(MappedFile file, size_t initCap, Error* status);
Error  lexerAnalyze(Lexer* lexer);
Error  lexerDestroy(Lexer* lexer, bool isAlloced);
Error  lexerPrintTokens(FILE* sink, Lexer* lexer);
Error  lexerVerify(Lexer* lexer);

#endif
