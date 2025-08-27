#ifndef _EXPRESSIONS_H
#define _EXPRESSIONS_H

#include "token.h"
#include "value.h"

enum ExpressionType {
  EXPRESSION_UNARY,
  EXPRESSION_BINARY,
  EXPRESSION_GROUP,
  EXPRESSION_CALL,
  EXPRESSION_GET,
  EXPRESSION_SET,
  EXPRESSION_LITERAL,
  EXPRESSION_ASSIGNMENT,
  EXPRESSION_ANON_FUN,
  EXPRESSION_STATIC,
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

struct Get {
  Expression* object;
  Token name;
};

struct Set {
  Expression* object;
  Token name;
  Expression* right;
};

struct Assignment {
  Token name;
  Expression* right;
};

struct AnonFunParams {
  size_t capacity;
  size_t count;
  StringView* xs;
};

struct AnonFun {
  struct AnonFunParams params;
  struct Statement* body; 
  Token* fun_kw;
};

struct Expression {
  enum ExpressionType type;
  union {
    struct Binary binary;
    struct Unary unary;
    Token literal;
    struct Group group;
    struct Call call;
    struct Get get;
    struct Set set;
    struct Assignment assignment;
    struct AnonFun anon_fun;
    Value evaluated;
  };
};

#endif
