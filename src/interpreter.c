#include "interpreter.h"
#include "parser.h"
#include "lexer.h"
#include "interpreter/scope.h"
#include "types/evaluation.h"
#include "error/runtime.h"

#include <assert.h>
#include <string.h>
#include <math.h>

static bool is_number(const Evaluation* e) {
  return e->type == EVAL_TYPE_DOUBLE;
}

static bool is_bool_convertable(const Evaluation* e) {
  switch (e->type) {
    case EVAL_TYPE_ERR:
      return false;
    default:
      return true;
  }
}

static bool as_bool(const Evaluation* e) {
  switch (e->type) {
    case EVAL_TYPE_DOUBLE:
      return e->dvalue > 0;
    case EVAL_TYPE_STRING_VIEW:
      return e->svvalue.len > 0;
    case EVAL_TYPE_BOOL:
      return e->bvalue;
    case EVAL_TYPE_ERR:
      // shut down warning, this is actually wrong but should be unreachable
      return false;
  }
}

static Evaluation evaluate_expression(Expression* expr);

static Evaluation evaluate_expression_literal_string(Expression* expr) {
  assert(expr->type == EXPRESSION_LITERAL && expr->literal.type == TOKEN_TYPE_STRING);

  Evaluation e;
  e.type = EVAL_TYPE_STRING_VIEW;
  e.svvalue = expr->literal.content;
  return e;
}

static Evaluation evaluate_expression_literal_double(Expression* expr) {
  assert(expr->type == EXPRESSION_LITERAL && expr->literal.type == TOKEN_TYPE_NUMBER);
  Number num = expr->literal.value;
  double val = NAN;
  if (num.decimal == 0) {
    val = (double)num.whole;
  } else {
    long temp = num.decimal;
    int numdigits = 0;
    while (temp > 0) {
      temp /= 10;
      ++numdigits;
    }

    val = num.whole + num.decimal / pow(10, numdigits);
  }

  Evaluation e;
  e.type = EVAL_TYPE_DOUBLE;
  e.dvalue = val;
  return e;
}

static Evaluation evaluate_expression_group(Expression* expr) {
  assert(expr->type == EXPRESSION_GROUP);
  return evaluate_expression(expr->group.child);
}

static Evaluation evaluate_expression_unary(Expression* expr) {
  assert(expr->type == EXPRESSION_UNARY);

  Evaluation e;

  Evaluation right = evaluate_expression(expr->unary.child);
  switch (expr->unary.operator.type) {
    case TOKEN_TYPE_MINUS:
      if (!is_number(&right)) {
        runtime_error(find_token(expr->unary.child), "Unary operation not permitted: operand is not a number");
        Evaluation e = {EVAL_TYPE_ERR};
        return e;
      }
      e.type = EVAL_TYPE_DOUBLE;
      e.dvalue = -right.dvalue;
    break;

    case TOKEN_TYPE_BANG:
      e.type = EVAL_TYPE_BOOL;
      e.bvalue = !as_bool(&right);
    break;

    default:
      fprintf(stderr, "Error unary expr with unrecognized token type");
      exit(1);
      break;
  }

  return e;
}

static Evaluation evaluate_expression_binary(Expression* expr) {
  assert(expr->type == EXPRESSION_BINARY);
  Evaluation e;

  Evaluation left = evaluate_expression(expr->binary.left);
  Evaluation right = evaluate_expression(expr->binary.right);

  if (!is_number(&left)) {
    runtime_error(find_token(expr->binary.left), "Binary operation not permitted: left operand is not a number");
    Evaluation e = {EVAL_TYPE_ERR};
    return e;
  }
  if (!is_number(&right)) {
    runtime_error(find_token(expr->binary.right), "Binary operation not permitted: right operand is not a number");
    Evaluation e = {EVAL_TYPE_ERR};
    return e;
  }

  switch (expr->binary.operator.type) {
    case TOKEN_TYPE_PLUS:
    case TOKEN_TYPE_MINUS:
    case TOKEN_TYPE_STAR:
    case TOKEN_TYPE_SLASH:
      e.type = EVAL_TYPE_DOUBLE;
      switch (expr->binary.operator.type) {
        case TOKEN_TYPE_PLUS:
          e.dvalue = left.dvalue + right.dvalue;
          break;
        case TOKEN_TYPE_MINUS:
          e.dvalue = left.dvalue - right.dvalue;
          break;
        case TOKEN_TYPE_STAR:
          e.dvalue = left.dvalue * right.dvalue;
          break;
        case TOKEN_TYPE_SLASH:
          if (right.dvalue == 0.0) {
            e.dvalue = NAN;
          } else {
            e.dvalue = left.dvalue / right.dvalue;
          }
          break;
        default:
        break;
      }
      break;

    case TOKEN_TYPE_LESS:
    case TOKEN_TYPE_LESS_EQUAL:
    case TOKEN_TYPE_GREATER:
    case TOKEN_TYPE_GREATER_EQUAL:
    case TOKEN_TYPE_EQUAL_EQUAL:
    case TOKEN_TYPE_BANG_EQUAL:
      e.type = EVAL_TYPE_BOOL;
      switch (expr->binary.operator.type) {
        case TOKEN_TYPE_LESS:
          e.bvalue = left.dvalue < right.dvalue;
        break;
        case TOKEN_TYPE_LESS_EQUAL:
          e.bvalue = left.dvalue <= right.dvalue;
        break;
        case TOKEN_TYPE_GREATER:
          e.bvalue = left.dvalue > right.dvalue;
        break;
        case TOKEN_TYPE_GREATER_EQUAL:
          e.bvalue = left.dvalue >= right.dvalue;
        break;
        case TOKEN_TYPE_EQUAL_EQUAL:
          e.bvalue = left.dvalue == right.dvalue;
        break;
        case TOKEN_TYPE_BANG_EQUAL:
          e.bvalue = left.dvalue != right.dvalue;
        break;
        default:
        break;
      }
      break;

    default:
      fprintf(stderr, "Error binary expr with unrecognized token type");
      exit(1);
  }

  return e;
}

static Evaluation evaluate_expression_assignment(Expression* expr) {
  Evaluation rhs = evaluate_expression(expr->assignment.right);
  StringView lexeme = expr->assignment.name.lexeme;

  if (!scope_replace(lexeme, &rhs)) {
    runtime_error(&expr->assignment.name, "Assignement failed. Variable must be declared with the 'var' keyword first"); 
    Evaluation e = {EVAL_TYPE_ERR};
    return e;
  }

  // returning it's now own stored value allows for chain assignments
  return rhs;
}

static Evaluation evaluate_expression(Expression* expr) {
  switch (expr->type) {
    case EXPRESSION_UNARY:
      return evaluate_expression_unary(expr);
    case EXPRESSION_BINARY:
      return evaluate_expression_binary(expr);
    case EXPRESSION_GROUP:
      return evaluate_expression_group(expr);
    case EXPRESSION_LITERAL:
      switch (expr->literal.type) {
        case TOKEN_TYPE_STRING:
          return evaluate_expression_literal_string(expr);
        case TOKEN_TYPE_NUMBER:
          return evaluate_expression_literal_double(expr);
        case TOKEN_TYPE_IDENTIFIER: {
          Evaluation* val = scope_get_val(expr->literal.lexeme);
          if (val == NULL) {
            StringView lexeme = expr->literal.lexeme;
            runtime_error(NULL, "Unresolved identifier: %.*s", lexeme.len, lexeme.str);
            Evaluation e = {EVAL_TYPE_ERR};
            return e;
          } else {
            return *val;
          }
        }
        break;
        case TOKEN_TYPE_KEYWORD:
          switch(expr->literal.keyword) {
            case RESERVED_KEYWORD_TRUE:
              {
                Evaluation e = {EVAL_TYPE_BOOL};
                e.bvalue = true;
                return e;
              }
            case RESERVED_KEYWORD_FALSE:
              {
                Evaluation e = {EVAL_TYPE_BOOL};
                e.bvalue = false;
                return e;
              }
            default:
              fprintf(stderr, "Keyword cannot be evaluated");
              exit(1);
          }
        break;
        default:
          fprintf(stderr, "Unimplemented token type literal");
          exit(1);
      }
      break;
    case EXPRESSION_ASSIGNMENT:
      return evaluate_expression_assignment(expr);
    break;
    default:
      fprintf(stderr, "Unimplemented expression type %d", expr->type);
      exit(1);
    break;
  }
}

static void evaluate_statement(Statement* stmt);

static void evaluate_statement_expr(Statement* stmt) {
  evaluate_expression(stmt->expr);
}

static void evaluate_statement_print(Statement* stmt) {
  Evaluation e = evaluate_expression(stmt->expr);

  // have some better printing in the future
  evaluation_pretty_print(&e);
}

static void evaluate_statement_var_decl(Statement* stmt) {
  Evaluation e = evaluate_expression(stmt->var_decl.expr);
  scope_insert(stmt->var_decl.identifier, &e);
}

static void evaluate_statement_block(Statement* stmt) {
  scope_new();
    for (size_t i = 0; i < stmt->block.num_stmts; ++i) {
      evaluate_statement(stmt->block.stmts[i]);   
    }
  scope_pop();
}

static void evaluate_statement_conditional(Statement* stmt) {
  Vector* condblocks = &stmt->cond.condblocks;
  for (size_t i = 0; i < condblocks->num_els; ++i) {
    struct ConditionalBlock* b = ((struct ConditionalBlock*)condblocks->els) + i;
    Expression* condition = b->condition;
    
    // else statement
    if (condition == NULL && i == condblocks->num_els - 1) {
      evaluate_statement(b->branch);
      return;
    }

    Evaluation e = evaluate_expression(condition);
    if (!is_bool_convertable(&e)) {
      runtime_error(NULL, "If statement condition can't be evaluated as boolean");
      exit(1);
    }

    if(as_bool(&e)) {
      evaluate_statement(b->branch);
      return;
    }
  }
}

static void evaluate_statement_while(Statement* stmt) {
  Evaluation e = evaluate_expression(stmt->while_loop.condition);
  if (!is_bool_convertable(&e)) {
    runtime_error(NULL, "While statement's condition doesn't evaluate to bool");
    return;
  }

  bool iterate = as_bool(&e);
  while (iterate) {
    evaluate_statement(stmt->while_loop.body);
    e = evaluate_expression(stmt->while_loop.condition);
    
    // i believe it's necessary because variables might change type
    if (!is_bool_convertable(&e)) {
      runtime_error(NULL, "While statement's condition doesn't evaluate to bool");
      return;
    }

    iterate = as_bool(&e);
  }
}

static void evaluate_statement(Statement* stmt) {
  switch (stmt->type) {
    case STATEMENT_EXPR:
      evaluate_statement_expr(stmt);
    break;
    case STATEMENT_PRINT_EXPR:
      evaluate_statement_print(stmt);
    break;
    case STATEMENT_VAR_DECL:
      evaluate_statement_var_decl(stmt);
    break;
    case STATEMENT_BLOCK:
      evaluate_statement_block(stmt);
    break;
    case STATEMENT_CONDITIONAL:
      evaluate_statement_conditional(stmt);
    break;
    case STATEMENT_WHILE:
      evaluate_statement_while(stmt);
    break;
  }
}

void evaluation_pretty_print(Evaluation* e) {
  switch(e->type) {
    case EVAL_TYPE_DOUBLE:
      printf("Double: %f\n", e->dvalue);
    break;
    case EVAL_TYPE_STRING_VIEW:
      printf("String: %.*s\n", (int)e->svvalue.len, e->svvalue.str);
    break;
    case EVAL_TYPE_BOOL:
      printf("Boolean: %s\n", e->bvalue ? "true" : "false");
    break;
    case EVAL_TYPE_ERR:
      printf("Error\n");
    break;
  }
}

void interpret(Statements stmts) {
  scope_new();

  for (size_t i = 0; i < stmts.num_stmts; ++i) {
    evaluate_statement(stmts.stmts[i]);
  }
}
