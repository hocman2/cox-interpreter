#ifndef _STATEMENTS_H
#define _STATEMENTS_H

#include "string_view.h"

enum StatementType {
  STATEMENT_EXPR,
  STATEMENT_PRINT_EXPR,
  STATEMENT_VAR_DECL,
  STATEMENT_FUN_DECL,
  STATEMENT_BLOCK,
  STATEMENT_CONDITIONAL,
  STATEMENT_WHILE,
};

typedef struct Statement Statement;

typedef struct {
  size_t count;
  size_t capacity;
  Statement* xs;
} Statements;

typedef struct Expression Expression;

typedef Expression* StatementExpression;

struct StatementVarDecl {
  Expression* expr;
  StringView identifier;
};

struct StatementFunParameters  {
  size_t capacity;
  size_t count;
  StringView* xs;
};

struct StatementFunDecl {
  StringView identifier;
  struct StatementFunParameters params;
  Statement* body;
};

typedef Statements StatementBlock;

struct ConditionalBlock {
  Expression* condition;
  Statement* branch;
};

struct StatementConditional {
  size_t count;
  size_t capacity;
  struct ConditionalBlock* xs;
};

struct StatementWhile {
  Expression* condition;
  Statement* body;
};

struct Statement {
  enum StatementType type;
  union {
    StatementExpression expr;
    struct StatementVarDecl var_decl;
    struct StatementFunDecl fun_decl;
    StatementBlock block;
    struct StatementConditional cond;
    struct StatementWhile while_loop;
  };
};

#endif
