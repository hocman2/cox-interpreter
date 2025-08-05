#ifndef _EXPRESSIONS_H
#define _EXPRESSIONS_H

#include "token.h"
#include "value.h"

enum ExpressionType {
  EXPRESSION_UNARY,
  EXPRESSION_BINARY,
  EXPRESSION_GROUP,
  EXPRESSION_CALL,
  EXPRESSION_LITERAL,
  EXPRESSION_ASSIGNMENT,
  EXPRESSION_EVALUATED,
};

typedef struct Expression Expression;

struct Binary {
  Expression* left;
  Expression* right;
  Token operator;
};

struct Unary {
  Expression* child;
  Token operator;
};

struct Group {
  Expression* child;
};

struct CallArguments {
  Expression** xs;
  size_t capacity;
  size_t count;
};

struct Call {
  Expression* callee;
  Token open_paren;
  struct CallArguments args;
};

struct Assignment {
  Token name;
  Expression* right;
};

struct Expression {
  enum ExpressionType type;
  union {
    struct Binary binary;
    struct Unary unary;
    Token literal;
    struct Group group;
    struct Call call;
    struct Assignment assignment;
    Value evaluated;
  };
};

#endif
