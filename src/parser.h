#ifndef _AST_H
#define _AST_H

#include "lexer.h"
#include "types/arena.h"
#include "types/vector.h"
#include "types/evaluation.h"

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
    Evaluation evaluated;
  };
};

enum StatementType {
  STATEMENT_EXPR,
  STATEMENT_PRINT_EXPR,
  STATEMENT_VAR_DECL,
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


typedef Expression* StatementExpression;

struct StatementVarDecl {
  Expression* expr;
  StringView identifier;
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
    StatementBlock block;
    struct StatementConditional cond;
    struct StatementWhile while_loop;
  };
};

struct Parser {
  Arena alloc;
  bool panic;
};

bool is_non_declarative_statement(Statement* stmt);
bool is_declarative_statement(Statement* stmt);

Token* find_token(Expression* expr);

// call before ast_build pls !!
void parser_init();
void parser_free(Statements* stmts);
bool parse(Token* tokens, size_t num_tokens, Statements* stmts);
void expression_pretty_print(Expression* expr);
void statement_pretty_print(Statement* stmt);

#endif
