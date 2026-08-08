#include "frontend/api/preparser.h"
#include <string.h>

#define CLASS_LIST() \
  X(None)            \
  X(Mage)            \
  X(Priest)          \
  X(Warrior)         \
  X(Warlock)         

typedef enum {
#define X(enm) enm,
CLASS_LIST()
#undef X
} Class;

// WARN: This list has both X and Y macros (for 1 and 2 classes respectively)
#define CLASSIFIED_TOKENS_LIST() \
  X(TOK_EMPOWER, Mage)           \
  X(TOK_NUM_LIT, Mage)           \
  X(TOK_MIRROR, Mage)            \
  X(TOK_HIT, Warrior)            \
  X(TOK_DUELL, Warrior)          \
  X(TOK_DUELR, Warrior)          \
  X(TOK_DUELLR, Warrior)         \
  X(TOK_WORTHY, Warrior)         \
  X(TOK_PUSHL, Warrior)          \
  X(TOK_PUSHR, Warrior)          \
  X(TOK_SHADOW, Priest)          \
  X(TOK_WHILE, Priest)           \
  X(TOK_UNITE, Priest)           \
  X(TOK_AND, Priest)             \
  Y(TOK_NOT, Priest, Warlock)    \
  X(TOK_UNTIL, Warlock)          \
  X(TOK_SHATTER, Warlock)        \
  X(TOK_OR, Warlock)

static bool isInvalidClass(TokenType type, Class curClass);
static const char* getClassStr(Class class);

bool preparse(Tokens* ts, Difficulty dif, Error* status) {
  Error err = OK;
  if ((err = dynArrVerify(ts)))
    RETURN_WITH_STATUS(err, false);

  bool allValid = true;
  Tokens newTs = (Tokens){};
  if ((err = dynArrInit(&newTs, ts->capacity, sizeof(Token), NULL)))
    RETURN_WITH_STATUS(err, false);

  _unused Class curClass = None;
  for (size_t i = 0; i < ts->count; i++) {
    Token* t = (Token*)dynArrGet(ts, i);
    switch (t->type) {
      case TOK_MAGE:    curClass = Mage;    continue;
      case TOK_PRIEST:  curClass = Priest;  continue;
      case TOK_WARRIOR: curClass = Warrior; continue;
      case TOK_WARLOCK: curClass = Warlock; continue;
      default: {
        if (dif != Easy) {
          bool isInv = isInvalidClass(t->type, curClass);
          if (isInv) {
            fprintf(stderr, 
                    "%zu:%zu [ERROR] %s isn't able to use \"%.*s\"\n"
                    "|\t%.*s\n",
                    t->lineN, (size_t)(t->pos - t->lineStart),
                    getClassStr(curClass), (int)t->len, t->pos,
                    (int)(strchr(t->lineStart, '\n') - t->lineStart), 
                    t->lineStart);
            allValid = false;
          } 
          t->isInvalidClass = isInv;
        }
        dynArrAppend(&newTs, t); break;
      }
    }
  }

  dynArrDestroy(ts, false);
  *ts = newTs;

  return allValid;
}

_unused static bool isInvalidClass(TokenType type, Class curClass) {
#define X(tok, class)           case tok: return class != curClass;
#define Y(tok, class, altClass) case tok: return class != curClass && altClass != curClass;
  switch (type) {
    CLASSIFIED_TOKENS_LIST()
    default: return false;
  }
#undef X
#undef Y
}

_unused static const char* getClassStr(Class class) {
  if (class == None)
    return "Unclassified";

  switch (class) {
#define X(enm) case enm: return #enm;
    CLASS_LIST()
#undef X
    default: return "UnknownClass";
  }
}
