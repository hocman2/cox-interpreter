#ifndef _AST_H
#define _AST_H

#include "lexer.h"
#include "types/arena.h"
#include "types/vector.h"
#include "types/value.h"
#include "types/statements.h"
#include "types/expressions.h"

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
