#include "interpreter.h"
#include "parser.h"
#include "lexer.h"
#include "interpreter/scope.h"
#include "types/value.h"
#include "types/string_view.h"
#include "error/runtime.h"

#include <assert.h>
#include <string.h>
#include <math.h>
#include <setjmp.h>

struct PendingReturn {
  Value value;
  bool env_init;
  jmp_buf env;
};
static struct PendingReturn pending_return = {{EVAL_TYPE_NIL}, false}; 

static Value evaluate_expression(Expression* expr);
static void evaluate_statement(Statement* stmt);

static Value evaluate_expression_literal_string(Expression* expr) {
  assert(expr->type == EXPRESSION_LITERAL && expr->literal.type == TOKEN_TYPE_STRING);

  Value e;
  e.type = EVAL_TYPE_STRING_VIEW;
  e.svvalue = expr->literal.content;
  return e;
}

static Value evaluate_expression_literal_double(Expression* expr) {
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

  Value e;
  e.type = EVAL_TYPE_DOUBLE;
  e.dvalue = val;
  return e;
}

static Value evaluate_expression_group(Expression* expr) {
  assert(expr->type == EXPRESSION_GROUP);
  return evaluate_expression(expr->group.child);
}

static Value evaluate_expression_call(Expression* expr) {
  Value fnvalue_evaluated;
  Value* fnvalue; 
  Expression* callee_expr = expr->call.callee;

  if (
    callee_expr->type == EXPRESSION_LITERAL && 
    callee_expr->literal.type == TOKEN_TYPE_IDENTIFIER
  ) {
    fnvalue = scope_get_val(callee_expr->literal.lexeme);
    if (fnvalue == NULL) {
      runtime_error(find_token(callee_expr), "Unresolved identifier as function name");
      return (Value){EVAL_TYPE_ERR};
    }
  } else {
    fnvalue_evaluated = evaluate_expression(callee_expr);
    fnvalue = &fnvalue_evaluated;
    if (fnvalue->type != EVAL_TYPE_FUN) {
      runtime_error(find_token(callee_expr), "Expression does not evaluate to a callable");
      return (Value){EVAL_TYPE_ERR};
    }
  }

  struct FunctionValue* fn = &fnvalue->fnvalue;
  if (expr->call.args.count < fn->params.count) {
    char errmsg[1024] = "Missing arguments: ";
    size_t cursor = strlen(errmsg);
    for (size_t i = expr->call.args.count; i < fn->params.count; ++i) {
      StringView param_name = fn->params.xs[i];
      snprintf(errmsg + cursor, param_name.len + 3, "\""SV_Fmt"\"", SV_Fmt_arg(param_name));
      cursor += param_name.len + 2;
      if (i < fn->params.count - 1) {
        snprintf(errmsg + cursor, 3, ", ");
        cursor += 2;
      }
    }
    snprintf(errmsg + cursor, strlen(" in function call") + 1, " in function call");
    runtime_error(&expr->call.open_paren, errmsg);
    return (Value){EVAL_TYPE_ERR};
  } else if (expr->call.args.count > fn->params.count) {
    runtime_error(&expr->call.open_paren, "Extraneous arguments in function call");
    return (Value){EVAL_TYPE_ERR};
  }

  scope_new();
  for (size_t i = 0; i < fn->params.count; ++i) {
    StringView param_name = fn->params.xs[i];
    Value arg = evaluate_expression(expr->call.args.xs[i]);
    scope_insert(param_name, &arg);
  }

  if (setjmp(pending_return.env) == 0) {
    pending_return.env_init = true;
    evaluate_statement(fn->body);
    return (Value){EVAL_TYPE_NIL};
  } else {
    pending_return.env_init = false;
    Value ret = pending_return.value;
    pending_return.value = (Value){EVAL_TYPE_NIL};
    return ret;
  }
}

static Value evaluate_expression_unary(Expression* expr) {
  assert(expr->type == EXPRESSION_UNARY);

  Value e;

  Value right = evaluate_expression(expr->unary.child);
  switch (expr->unary.operator.type) {
    case TOKEN_TYPE_MINUS:
      if (!convert_to(&right, EVAL_TYPE_DOUBLE)) {
        runtime_error(find_token(expr->unary.child), "Unary operation not permitted: operand is not a number");
        Value e = {EVAL_TYPE_ERR};
        return e;
      }

      e.type = EVAL_TYPE_DOUBLE;
      e.dvalue = -right.dvalue;
    break;

    case TOKEN_TYPE_BANG:
      e.type = EVAL_TYPE_BOOL;
      convert_to(&right, EVAL_TYPE_BOOL);
      e.bvalue = !right.bvalue;
    break;

    default:
      fprintf(stderr, "Error unary expr with unrecognized token type");
      exit(1);
      break;
  }

  return e;
}

static Value binary_eval_member(Expression* expr, bool eval_left, enum ValueType expected_type) {
  Expression* to_eval = (eval_left) ? expr->binary.left : expr->binary.right;
  Value eval = evaluate_expression(to_eval);

  if (!convert_to(&eval, expected_type)) {
    runtime_error(
      find_token(to_eval),
      "Binary operation not permitted: %s operand is not convertible to %s",
      (eval_left) ? "left" : "right", eval_type_to_str(expected_type)
    );

    Value e = {EVAL_TYPE_ERR};
    return e;
  }

  return eval;
}

static Value evaluate_expression_binary(Expression* expr) {
  assert(expr->type == EXPRESSION_BINARY);

  Value e;

  switch (expr->binary.operator.type) {
    // Arithmetic operators evaluation
    case TOKEN_TYPE_PLUS:
    case TOKEN_TYPE_MINUS:
    case TOKEN_TYPE_STAR:
    case TOKEN_TYPE_SLASH: {
      Value left = binary_eval_member(expr, true, EVAL_TYPE_DOUBLE);
      Value right = binary_eval_member(expr, false, EVAL_TYPE_DOUBLE);

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
    }
      break;

    // Comparison operators evaluation
    case TOKEN_TYPE_LESS:
    case TOKEN_TYPE_LESS_EQUAL:
    case TOKEN_TYPE_GREATER:
    case TOKEN_TYPE_GREATER_EQUAL:
    case TOKEN_TYPE_EQUAL_EQUAL:
    case TOKEN_TYPE_BANG_EQUAL: {
      Value left = binary_eval_member(expr, true, EVAL_TYPE_DOUBLE);
      Value right = binary_eval_member(expr, false, EVAL_TYPE_DOUBLE);

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
    }
      break;

    case TOKEN_TYPE_KEYWORD:
      // Logic operator evaluations
      e.type = EVAL_TYPE_BOOL;
      switch (expr->binary.operator.keyword) {
        case RESERVED_KEYWORD_OR: {
          Value left = binary_eval_member(expr, true, EVAL_TYPE_BOOL);
          if (left.bvalue) {
            e.bvalue = true;
          } else {
            Value right = binary_eval_member(expr, false, EVAL_TYPE_BOOL);
            e.bvalue = right.bvalue;
          }
        }
          break;
          
        case RESERVED_KEYWORD_AND: {
          Value left = binary_eval_member(expr, true, EVAL_TYPE_BOOL);
          if (!left.bvalue) {
            e.bvalue = false;
          } else {
            Value right = binary_eval_member(expr, false, EVAL_TYPE_BOOL);
            e.bvalue = right.bvalue;
          }
        }
          break;
        default:
          fprintf(stderr, "Error binary expr with unrecognized keyword type");
          exit(1);
      }
      break;

    default:
      fprintf(stderr, "Error binary expr with unrecognized token type");
      exit(1);
  }

  return e;
}

static Value evaluate_expression_assignment(Expression* expr) {
  Value rhs = evaluate_expression(expr->assignment.right);
  StringView lexeme = expr->assignment.name.lexeme;

  if (!scope_replace(lexeme, &rhs)) {
    runtime_error(&expr->assignment.name, "Assignement failed. Variable must be declared with the 'var' keyword first"); 
    Value e = {EVAL_TYPE_ERR};
    return e;
  }

  // returning it's now own stored value allows for chain assignments
  return rhs;
}

static Value evaluate_expression(Expression* expr) {
  switch (expr->type) {
    case EXPRESSION_EVALUATED:
      return expr->evaluated;
    case EXPRESSION_UNARY:
      return evaluate_expression_unary(expr);
    case EXPRESSION_BINARY:
      return evaluate_expression_binary(expr);
    case EXPRESSION_GROUP:
      return evaluate_expression_group(expr);
    case EXPRESSION_CALL:
      return evaluate_expression_call(expr);
    case EXPRESSION_LITERAL:
      switch (expr->literal.type) {
        case TOKEN_TYPE_STRING:
          return evaluate_expression_literal_string(expr);
        case TOKEN_TYPE_NUMBER:
          return evaluate_expression_literal_double(expr);
        case TOKEN_TYPE_IDENTIFIER: {
          Value* val = scope_get_val(expr->literal.lexeme);
          if (val == NULL) {
            StringView lexeme = expr->literal.lexeme;
            runtime_error(NULL, "Unresolved identifier: "SV_Fmt, SV_Fmt_arg(lexeme));
            Value e = {EVAL_TYPE_ERR};
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
                Value e = {EVAL_TYPE_BOOL};
                e.bvalue = true;
                return e;
              }
            case RESERVED_KEYWORD_FALSE:
              {
                Value e = {EVAL_TYPE_BOOL};
                e.bvalue = false;
                return e;
              }
            case RESERVED_KEYWORD_NIL:
                return (Value){EVAL_TYPE_NIL};
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

static void evaluate_statement_expr(Statement* stmt) {
  evaluate_expression(stmt->expr);
}

static void evaluate_statement_print(Statement* stmt) {
  Value e = evaluate_expression(stmt->expr);

  // have some better printing in the future
  evaluation_pretty_print(&e);
}

static void evaluate_statement_var_decl(Statement* stmt) {
  Value e = evaluate_expression(stmt->var_decl.expr);
  scope_insert(stmt->var_decl.identifier, &e);
}

static void evaluate_statement_fun_decl(Statement* stmt) {
  struct FunctionValue fnval = {0};
  fnval.body = stmt->fun_decl.body; 
  vector_copy(fnval.params, stmt->fun_decl.params);

  Value fn = {0};
  fn.type = EVAL_TYPE_FUN;
  fn.fnvalue = fnval;
  scope_insert(stmt->fun_decl.identifier, &fn);
}

static void evaluate_statement_block(Statement* stmt) {
  scope_new();
  for (size_t i = 0; i < stmt->block.count; ++i) {
    Statement* s = stmt->block.xs + i;
    evaluate_statement(s);   
  }
  scope_pop();
}

static void evaluate_statement_conditional(Statement* stmt) {
  for (size_t i = 0; i < stmt->cond.count; ++i) {
    struct ConditionalBlock* b = stmt->cond.xs + i;
    Expression* condition = b->condition;
    
    // else statement
    if (condition == NULL && i == stmt->cond.count - 1) {
      evaluate_statement(b->branch);
      return;
    }

    Value e = evaluate_expression(condition);
    if (!convert_to(&e, EVAL_TYPE_BOOL)) {
      runtime_error(NULL, "If statement condition can't be evaluated as boolean");
      return;
    }

    if(e.bvalue) {
      evaluate_statement(b->branch);
      return;
    }
  }
}

static void evaluate_statement_while(Statement* stmt) {
  Value e = evaluate_expression(stmt->while_loop.condition);

  if (!convert_to(&e, EVAL_TYPE_BOOL)) {
    runtime_error(NULL, "While loop condition does not evaluate to bool");
    return;
  }

  bool iterate = e.bvalue;
  while (iterate) {
    evaluate_statement(stmt->while_loop.body);
    e = evaluate_expression(stmt->while_loop.condition);
    
    // i believe it's necessary because variables might change type
    if (!convert_to(&e, EVAL_TYPE_BOOL)) {
      runtime_error(NULL, "While loop condition does not evaluate to bool");
      return;
    }

    iterate = e.bvalue;
  }
}

static void evaluate_statement_return(Statement* stmt) {
  if (pending_return.env_init) {
    pending_return.value = evaluate_expression(stmt->ret);
    longjmp(pending_return.env, 1);
  } else {
    runtime_error(find_token(stmt->ret), "Return statement must be used inside a function body");
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
    case STATEMENT_FUN_DECL:
      evaluate_statement_fun_decl(stmt);
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
    case STATEMENT_RETURN:
      evaluate_statement_return(stmt);
    break;
  }
}

void evaluation_pretty_print(Value* e) {
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
    case EVAL_TYPE_FUN:
      printf("Function\n");
    break;
    case EVAL_TYPE_NIL:
      printf("Nil\n");
    break;
    case EVAL_TYPE_ERR:
      printf("Error\n");
    break;
  }
}

void interpret(Statements stmts) {
  scope_new();

  for (size_t i = 0; i < stmts.count; ++i) {
    Statement* stmt = stmts.xs + i;
    evaluate_statement(stmt);
  }

  scope_pop();
}
