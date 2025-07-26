#include <stdio.h>
#include <assert.h>
#include <setjmp.h>

#include "parser.h"
#include "types/arena.h"
#include "error/analysis.h"

#define MAX_EXPR 1000

static jmp_buf parse_error_jump;
static struct Parser parser;

static void statements_init(Statements* s) {
  s->_cap = 1;
  s->num_stmts = 0;
  s->stmts = malloc(sizeof(Statement*) * s->_cap);
}

static void statements_push(Statements* stmts, Statement* stmt) {
  if (stmts->num_stmts == stmts->_cap) {
    stmts->_cap *= 2;
    stmts->stmts = realloc(stmts->stmts, sizeof(Statement*) * stmts->_cap);
  }

  stmts->stmts[stmts->num_stmts] = stmt;
  stmts->num_stmts += 1;
}

bool is_non_declarative_statement(Statement* stmt) {
  switch (stmt->type) {
    case STATEMENT_VAR_DECL:
      return false;
    default:
      return true;
  }
}

bool is_declarative_statement(Statement* stmt) {
  return !is_non_declarative_statement(stmt);
}

Token* find_token(Expression* expr) {
  switch (expr->type) {
    case EXPRESSION_LITERAL:
      return &expr->literal;
    case EXPRESSION_GROUP:
      return find_token(expr->group.child);
    break;
    case EXPRESSION_UNARY:
      return find_token(expr->unary.child);
    break;
    case EXPRESSION_ASSIGNMENT:
      return &expr->assignment.name;
    break;
    case EXPRESSION_BINARY:
      // a choice must be made ...
      return find_token(expr->binary.left);
    break;
  }
}

void parser_init() {
  Arena areana = arena_init(sizeof(Expression) * MAX_EXPR, alignof(Expression));
  parser.alloc = areana;
  parser.panic = false;
}

void parser_free() {
  arena_free(&parser.alloc);
}

static bool is_factor_op(Token* token) {
  if (!token) return false;
  int t = token->type;

  return 
    t == TOKEN_TYPE_STAR ||
    t == TOKEN_TYPE_SLASH
    ;
}

static bool is_term_op(Token* token) {
  if (!token) return false;
  int t = token->type;

  return 
    t == TOKEN_TYPE_PLUS ||
    t == TOKEN_TYPE_MINUS
    ;
}

static bool is_comp_op(Token* token) {
  if (!token) return false;
  int t = token->type;

  return 
    t == TOKEN_TYPE_GREATER ||
    t == TOKEN_TYPE_GREATER_EQUAL ||
    t == TOKEN_TYPE_LESS ||
    t == TOKEN_TYPE_LESS_EQUAL 
    ;
}

static bool is_equality_op(Token* token) {
  if (!token) return false;
  int t = token->type;

  return 
    t == TOKEN_TYPE_BANG_EQUAL ||
    t == TOKEN_TYPE_EQUAL_EQUAL 
    ;
}

static bool is_binary_op(Token* token) {
  return 
    is_equality_op(token) ||
    is_comp_op(token) ||
    is_term_op(token) ||
    is_factor_op(token);

}

static bool is_unary_op(Token* token) {
  if (!token) return false;
  int t = token->type;

  return
    t == TOKEN_TYPE_BANG ||
    t == TOKEN_TYPE_MINUS
    ;
}

struct TokensCursor {
  Token* current;
  size_t remaining;
};

static bool is_at_end(struct TokensCursor* cursor) {
  return cursor->remaining <= 1; 
}

static Token* token_at(struct TokensCursor* cursor) { return cursor->current; }

static Token* next_token(struct TokensCursor* cursor) {
  if (is_at_end(cursor)) return cursor->current;
  else return cursor->current + 1;
}

static Token* previous_token(struct TokensCursor* cursor) {
  return cursor->current - 1;
}

static bool is_keyword(struct TokensCursor* cursor, enum ReservedKeywordType expected_kw) {
  Token* t = token_at(cursor);
  return t->type == TOKEN_TYPE_KEYWORD && t->keyword == expected_kw;
}

static bool token_is_keyword(Token* t, enum ReservedKeywordType expected_kw) {
  return t->type == TOKEN_TYPE_KEYWORD && t->keyword == expected_kw;
}

static struct TokensCursor* advance(struct TokensCursor* cursor) {
  if (token_at(cursor)->type == TOKEN_TYPE_EOF) return cursor;

  cursor->current += 1;
  cursor->remaining -= 1;

  return cursor;
}

static void recover(struct TokensCursor* cursor) {
  Token* pv = previous_token(cursor);
  Token* t;
  while(t = token_at(cursor), !is_at_end(cursor)) {
    if (pv->type == TOKEN_TYPE_SEMICOLON) {
      parser.panic = false;
      return;
    }
    
    if (t->type == TOKEN_TYPE_KEYWORD) {
      switch (t->keyword) {
        case RESERVED_KEYWORD_CLASS:
        case RESERVED_KEYWORD_FUN:
        case RESERVED_KEYWORD_IF:
        case RESERVED_KEYWORD_ELSE:
        case RESERVED_KEYWORD_FOR:
        case RESERVED_KEYWORD_VAR:
        case RESERVED_KEYWORD_WHILE:
        case RESERVED_KEYWORD_RETURN:
          parser.panic = false;
          return;
        default: 
          break;
      }
    }

    pv = t;
    advance(cursor);
  }

  // no more statements, recovery failed
  parser.panic = true;
}

static Expression* parse_expression(struct TokensCursor* cursor);
static Statement* parse_statement(struct TokensCursor* cursor);


static void set_panic(struct TokensCursor* cursor) {
  parser.panic = true;
  // unwind the stack, ditching the currently parsed statement
  longjmp(parse_error_jump, 1);
}


// Consume a token, failure to do so interrupts the current statement's parsing and logs error
static Token* consume(struct TokensCursor* cursor, enum TokenType expected_type, const char* error_msg) {
  Token* t = token_at(cursor);
  if (!t || t->type != expected_type) { 
    syntax_error(token_at(cursor), error_msg);
    set_panic(cursor);
  } else {
    advance(cursor);
  }

  return t;
}

// Consume a keyword, failure to do so interrupts the current statement's parsing and logs error
static void consume_keyword(struct TokensCursor* cursor, enum ReservedKeywordType expected_kw) {
  if (!is_keyword(cursor, expected_kw)) {
    set_panic(cursor);
    internal_logic_error(token_at(cursor), "Missing expected keyword");
  } else {
    advance(cursor);
  }
}

static Expression* parse_primary(struct TokensCursor* cursor) {
  if (is_at_end(cursor)) return NULL;

  Expression* expr = arena_alloc(&parser.alloc, sizeof(Expression));

  Token* token = token_at(cursor);
  switch (token->type) {
    case TOKEN_TYPE_STRING:
    case TOKEN_TYPE_KEYWORD:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_TYPE_NUMBER:
      expr->type = EXPRESSION_LITERAL;
      expr->literal = *token;
      advance(cursor);
    break;
    case TOKEN_TYPE_LEFT_PAREN:
      expr->type = EXPRESSION_GROUP;
      expr->group.child = parse_expression(advance(cursor)); 
      consume(cursor, TOKEN_TYPE_RIGHT_PAREN, "Missing closing parentheses");
    break;

    default:
      // special treatement for lonely binary opeartor
      if (is_binary_op(token)) {
        syntax_error(token_at(cursor), "Binary operator missing left operand");
        set_panic(cursor);
        return NULL; // doesn't matter
      } else {
        // ignore token
        advance(cursor);
      }
    break;
  }

  return expr;
}

static Expression* parse_unary(struct TokensCursor* cursor) {
  if (is_unary_op(token_at(cursor))) {
    Expression* expr = arena_alloc(&parser.alloc, sizeof(Expression)); 
    expr->type = EXPRESSION_UNARY;
    expr->unary.operator = *token_at(cursor);
    expr->unary.child = parse_unary(advance(cursor));
    return expr;
  } else {
    return parse_primary(cursor);
  }
}

static Expression* parse_factor(struct TokensCursor* cursor) {
  Expression* expr = parse_unary(cursor);

  while (is_factor_op(token_at(cursor))) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_unary(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_term(struct TokensCursor* cursor) {
  Expression* expr = parse_factor(cursor);

  while (is_term_op(token_at(cursor))) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_factor(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_comparison(struct TokensCursor* cursor) {
  Expression* expr = parse_term(cursor);

  while (is_comp_op(token_at(cursor))) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_term(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_equality(struct TokensCursor* cursor) {
  Expression* expr = parse_comparison(cursor);

  while (is_equality_op(token_at(cursor))) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_comparison(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_logical_and(struct TokensCursor* cursor) {
  Expression* expr = parse_equality(cursor);
  
  while (is_keyword(cursor, RESERVED_KEYWORD_AND)) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_equality(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_logical_or(struct TokensCursor* cursor) {
  Expression* expr = parse_logical_and(cursor);
  
  while (is_keyword(cursor, RESERVED_KEYWORD_OR)) {
    Expression* bin = arena_alloc(&parser.alloc, sizeof(Expression));
    bin->type = EXPRESSION_BINARY;
    bin->binary.operator = *token_at(cursor);
    bin->binary.left = expr;
    bin->binary.right = parse_logical_and(advance(cursor));

    expr = bin;
  }

  return expr;
}

static Expression* parse_assignment(struct TokensCursor* cursor) {
  Expression* expr = parse_logical_or(cursor);

  Token* t = token_at(cursor);
  if (t->type == TOKEN_TYPE_EQUAL) {
    Token name = expr->literal;
    expr->type = EXPRESSION_ASSIGNMENT;
    expr->assignment.name = name;
    expr->assignment.right = parse_assignment(advance(cursor));
  }

  return expr;
}

static Expression* parse_expression(struct TokensCursor* cursor) {
  return parse_assignment(cursor);
}

static Statement* parse_statement_non_decl(struct TokensCursor* cursor);

static Statement* parse_statement_expr(struct TokensCursor* cursor) {
  Statement* stmt = arena_alloc(&parser.alloc, sizeof(Statement));
  stmt->type = STATEMENT_EXPR;
  stmt->expr = parse_expression(cursor);

  consume(cursor, TOKEN_TYPE_SEMICOLON, "Expect ';' after expression");

  return stmt;
}

static Statement* parse_statement_print(struct TokensCursor* cursor) {
  consume_keyword(cursor, RESERVED_KEYWORD_PRINT);
  Statement* stmt = parse_statement_expr(cursor); 
  stmt->type = STATEMENT_PRINT_EXPR;
  return stmt;
}

static Statement* parse_statement_var_decl(struct TokensCursor* cursor) {
  consume_keyword(cursor, RESERVED_KEYWORD_VAR);

  Token* identifier = consume(cursor, TOKEN_TYPE_IDENTIFIER, "Expect identifier after 'var' keyword");
  consume(cursor, TOKEN_TYPE_EQUAL, "Expect '=' after identifier");
  Statement* stmt = parse_statement_expr(cursor);

  // override the type and steal the expression
  stmt->type = STATEMENT_VAR_DECL;
  Expression* expr = stmt->expr;
  stmt->var_decl.identifier = identifier->lexeme;
  stmt->var_decl.expr = expr;

  return stmt;
}

static Statement* parse_statement_block(struct TokensCursor* cursor) {
  Statement* stmt_block = arena_alloc(&parser.alloc, sizeof(Statement));
  stmt_block->type = STATEMENT_BLOCK;
  statements_init(&(stmt_block->block));
  
  Token* t;
  while (
    t = token_at(cursor), 
    t->type != TOKEN_TYPE_RIGHT_BRACE && t->type != TOKEN_TYPE_EOF
  ) {
    Statement* stmt = parse_statement(cursor);
    statements_push(&(stmt_block->block), stmt);
  }

  consume(cursor, TOKEN_TYPE_RIGHT_BRACE, "Missing closing curly brace");

  return stmt_block;
}

static Statement* parse_statement_conditional(struct TokensCursor* cursor) {
  Statement* s = arena_alloc(&parser.alloc, sizeof(Statement));
  s->type = STATEMENT_CONDITIONAL;
  s->cond.condblocks = vector_new(1, sizeof(struct ConditionalBlock));
  Vector* condblocks = &(s->cond.condblocks);

  Expression* cond = parse_expression(cursor);
  Statement* ifstmt = parse_statement_non_decl(cursor);

  struct ConditionalBlock block = {cond, ifstmt};
  vector_push(condblocks, (void*)&block);

  while (is_keyword(cursor, RESERVED_KEYWORD_ELSE)) {
    if (token_is_keyword(next_token(cursor), RESERVED_KEYWORD_IF)) {
      advance(cursor); // skip if kw
      advance(cursor); // jump to condition
      block.condition = parse_expression(cursor);
      block.branch = parse_statement_non_decl(cursor);
      vector_push(condblocks, (void*)&block);
      // not that we are not calling parse_statement_conditional
      // recursively because it would needlessly allocate a full if statement
      // that would be discarded
    } else {
      advance(cursor);
      block.condition = NULL;
      block.branch = parse_statement_non_decl(cursor);
      vector_push(condblocks, (void*)&block);
    }
  }

  return s;
}

static Statement* parse_statement_while(struct TokensCursor* cursor) {
  Statement* s = arena_alloc(&parser.alloc, sizeof(Statement));
  s->type = STATEMENT_WHILE;
  s->while_loop.condition = parse_expression(cursor);
  s->while_loop.body = parse_statement(cursor);
  return s;
}

static Statement* parse_statement_non_decl(struct TokensCursor* cursor) {
  Token* t = token_at(cursor);
  switch(t->type) {
    case TOKEN_TYPE_LEFT_BRACE:
      return parse_statement_block(advance(cursor));
    break;
    case TOKEN_TYPE_KEYWORD:
      switch (t->keyword) {
        case RESERVED_KEYWORD_PRINT:
          return parse_statement_print(cursor);
        case RESERVED_KEYWORD_IF:
          return parse_statement_conditional(advance(cursor));
        case RESERVED_KEYWORD_ELSE:
          syntax_error(token_at(cursor), "else statement must follow an if block");
          set_panic(cursor);
        case RESERVED_KEYWORD_WHILE:
          return parse_statement_while(advance(cursor));
        default:
        break;
      }
    break;
    default:
    break;
  }

  return parse_statement_expr(cursor);
}

static Statement* parse_statement(struct TokensCursor* cursor) {
  Statement* stmt;
  if (is_keyword(cursor, RESERVED_KEYWORD_VAR)) {
    stmt = parse_statement_var_decl(cursor);
  } else {
    stmt = parse_statement_non_decl(cursor); 
  }

  if (parser.panic) {
    recover(cursor);
    return NULL;
  }

  return stmt;
}

bool parse(Token* tokens, size_t num_tokens, Statements* stmts) {
  assert(stmts && "A valid Statements pointer is mandatory in parse()");

  // init of some stuff
  struct TokensCursor cursor = {tokens, num_tokens};

  parser.panic = false;

  statements_init(stmts);

  // actual parsing
  while (token_at(&cursor)->type != TOKEN_TYPE_EOF) {
    if (setjmp(parse_error_jump) == 0) {
      Statement* stmt = parse_statement(&cursor);
      if (!stmt) continue;

      statements_push(stmts, stmt);
    } else {
      recover(&cursor);
      if (is_at_end(&cursor)) break;
    }
  }

  return !parser.panic;
}

void expression_pretty_print(Expression* expr) {
  if (!expr) {
    printf("NULL");
    return;
  }

  switch (expr->type) {
    case EXPRESSION_LITERAL:
      switch (expr->literal.type) {
        case TOKEN_TYPE_NUMBER:
          printf("%lu.%lu", expr->literal.value.whole, expr->literal.value.decimal);
          break;
        case TOKEN_TYPE_STRING:
          printf("%.*s", (int)expr->literal.content.len, expr->literal.content.str);
          break;
        default:
          printf("%.*s", (int)expr->literal.lexeme.len, expr->literal.lexeme.str);
      }
      break;
    case EXPRESSION_GROUP:
      printf("(group ");
      expression_pretty_print(expr->group.child);
      printf(")");
      break;
    case EXPRESSION_UNARY:
      printf("(%.*s ", (int)expr->unary.operator.lexeme.len, expr->unary.operator.lexeme.str);
      expression_pretty_print(expr->unary.child);
      printf(")");
      break;
    case EXPRESSION_BINARY:
      printf("(%.*s ", (int)expr->binary.operator.lexeme.len, expr->binary.operator.lexeme.str);
      expression_pretty_print(expr->binary.left);
      printf(" ");
      expression_pretty_print(expr->binary.right);
      printf(")");
      break;
    case EXPRESSION_ASSIGNMENT:
      printf("(= "SV_Fmt" ", SV_Fmt_arg(expr->assignment.name.lexeme));
      expression_pretty_print(expr->assignment.right);
      printf(")");
    break;
    default:
      break;
  }
}

void statement_pretty_print(Statement* stmt) {
  if (!stmt) {
    printf("NULL");
    return;
  }

  switch (stmt->type) {
    case STATEMENT_EXPR:
      printf("STATEMENT EXPR: ");
      expression_pretty_print(stmt->expr);
      printf("\n");
    break;
    case STATEMENT_PRINT_EXPR:
      printf("STATEMENT PRINT: ");
      expression_pretty_print(stmt->expr);
      printf("\n");
    break;
    case STATEMENT_VAR_DECL:
      printf("STATEMENT VAR DECLARATION: ");
      {
        StringView id = stmt->var_decl.identifier;
        printf("(Identifier => %.*s ; Value => ", (int)id.len, id.str);
      }
      expression_pretty_print(stmt->var_decl.expr);
      printf(")\n");
    break;
    case STATEMENT_BLOCK:
      printf("STATEMENT BLOCK: \n");
      for (size_t i = 0; i < stmt->block.num_stmts; ++i) {
        printf("\t");
        statement_pretty_print(stmt->block.stmts[i]);
      }
    break;
    case STATEMENT_CONDITIONAL:
      printf("STATEMENT CONDITIONAL: \n");
      Vector* v = &(stmt->cond.condblocks);
      for (size_t i = 0; i < v->num_els; ++i) {
        struct ConditionalBlock* b = ((struct ConditionalBlock*)v->els) + i;
        printf("\tCondition: ");
        expression_pretty_print(b->condition);
        printf("\n");
        printf("\tBranch: ");
        statement_pretty_print(b->branch);
        printf("\n");
      }
    break;
    case STATEMENT_WHILE:
      printf("STATEMENT WHILE: \n");
      printf("\tCondition: ");
      expression_pretty_print(stmt->while_loop.condition);
      printf("\n");
      printf("\tBody: ");
      statement_pretty_print(stmt->while_loop.body);
      printf("\n");
    break;
  }
}

