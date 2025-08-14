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

// Small data structure that stores return value of a function across nested recursive calls
struct PendingReturn {
  Value value;
  bool await_return;
  bool should_return;
};
static struct PendingReturn pending_return = {{EVAL_TYPE_NIL}, false, false}; 
static Value take_return() {
  pending_return.await_return = false;
  pending_return.should_return = false;
  Value ret = pending_return.value;
  pending_return.value = value_new_nil();
  return ret;
}
static void set_return(Value v) {
  pending_return.should_return = true;
  pending_return.value = v;
}

static bool should_return() {
  return pending_return.should_return && pending_return.await_return;
}

static Value evaluate_expression(Expression* expr);
static void evaluate_statement(Statement* stmt);
static void evaluate_statement_block(Statement* stmt);

static Value evaluate_expression_literal_string(Expression* expr) {
  assert(expr->type == EXPRESSION_LITERAL && expr->literal.type == TOKEN_TYPE_STRING);
  return value_new_stringview(expr->literal.content);
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

  return value_new_double(val);
}

static Value evaluate_expression_group(Expression* expr) {
  assert(expr->type == EXPRESSION_GROUP);
  return evaluate_expression(expr->group.child);
}

static Value evaluate_expression_call(Expression* expr) {
  Value fnvalue_evaluated;
  Value* fnvalue; 
  Expression* callee_expr = expr->call.callee;

  // Resolve the callee as a scope value or an callable expression
  if (
    callee_expr->type == EXPRESSION_LITERAL && 
    callee_expr->literal.type == TOKEN_TYPE_IDENTIFIER
  ) {
    fnvalue = scope_get_val_ref(callee_expr->literal.lexeme);
    if (fnvalue == NULL) {
      runtime_error(find_token(callee_expr), "Unresolved identifier as function name");
      return value_new_err();
    }
  } else {
    fnvalue_evaluated = evaluate_expression(callee_expr);
    fnvalue = &fnvalue_evaluated;
    if (fnvalue->type != EVAL_TYPE_FUN) {
      runtime_error(find_token(callee_expr), "Expression does not evaluate to a callable");
      return value_new_err();
    }
  }

  // Check arguments arity
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
    return value_new_err();
  } else if (expr->call.args.count > fn->params.count) {
    runtime_error(&expr->call.open_paren, "Extraneous arguments in function call");
    return value_new_err();
  }

  scope_swap(fn->capture);
  scope_new();
  // bind the parameters
  for (size_t i = 0; i < fn->params.count; ++i) {
    StringView param_name = fn->params.xs[i];
    Value arg = evaluate_expression(expr->call.args.xs[i]);
    scope_insert(param_name, &arg);
  }

  pending_return.await_return = true;
  evaluate_statement_block(fn->body);
  Value ret = take_return();

  scope_pop();
  scope_restore();
  return ret;
}

static Value evaluate_expression_unary(Expression* expr) {
  assert(expr->type == EXPRESSION_UNARY);

  Value right = evaluate_expression(expr->unary.child);
  switch (expr->unary.operator.type) {
    case TOKEN_TYPE_MINUS:
      if (!convert_to(&right, EVAL_TYPE_DOUBLE)) {
        runtime_error(find_token(expr->unary.child), "Unary operation not permitted: operand is not a number");
        return value_new_err();
      }

      return value_new_double(-right.dvalue);
    break;

    case TOKEN_TYPE_BANG:
      if (!convert_to(&right, EVAL_TYPE_BOOL)) {
        runtime_error(find_token(expr->unary.child), "Unary operation not permitted: operand is not convertible to boolean");
        return value_new_err();
      }

      return value_new_bool(!right.bvalue);
    break;

    default:
      fprintf(stderr, "Error unary expr with unrecognized token type");
      exit(1);
      break;
  }

  // unreachable
  return value_new_nil();
}

// helper function
static Value binary_eval_member(Expression* expr, bool eval_left, enum ValueType expected_type) {
  Expression* to_eval = (eval_left) ? expr->binary.left : expr->binary.right;
  Value eval = evaluate_expression(to_eval);

  if (!convert_to(&eval, expected_type)) {
    runtime_error(
      find_token(to_eval),
      "Binary operation not permitted: %s operand is not convertible to %s",
      (eval_left) ? "left" : "right", eval_type_to_str(expected_type)
    );

    return value_new_err();
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

static Value evaluate_expression_anon_fun(Expression* expr) {
  Value fn = value_new_fun(expr->anon_fun.body, expr->anon_fun.params.xs, expr->anon_fun.params.count, scope_get_ref());
  return fn;
}

static Value evaluate_expression_assignment(Expression* expr) {
  Value rhs = evaluate_expression(expr->assignment.right);
  StringView lexeme = expr->assignment.name.lexeme;

  if (!scope_replace(lexeme, &rhs)) {
    runtime_error(&expr->assignment.name, "Assignement failed. Variable must be declared with the 'var' keyword first"); 
    return value_new_err();
  }

  // returning it's now own stored value allows for chain assignments
  return rhs;
}

static Value evaluate_expression(Expression* expr) {
  switch (expr->type) {
    case EXPRESSION_STATIC:
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
          Value val = scope_get_val_copy(expr->literal.lexeme);
          if (val.type == EVAL_TYPE_ERR) {
            StringView lexeme = expr->literal.lexeme;
            runtime_error(NULL, "Unresolved identifier: "SV_Fmt, SV_Fmt_arg(lexeme));
          }
          return val;
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
    case EXPRESSION_ANON_FUN:
      return evaluate_expression_anon_fun(expr);
    break;
    default:
      fprintf(stderr, "Unimplemented expression type %d", expr->type);
      exit(1);
    break;
  }
}

static void evaluate_statement_expr(Statement* stmt) {
  Value v = evaluate_expression(stmt->expr);
  value_scopeexit(&v);
}

static void evaluate_statement_print(Statement* stmt) {
  Value e = evaluate_expression(stmt->expr);

  // have some better printing in the future
  evaluation_pretty_print(&e);
  value_scopeexit(&e);
}

static void evaluate_statement_var_decl(Statement* stmt) {
  Value e = evaluate_expression(stmt->var_decl.expr);
  scope_insert(stmt->var_decl.identifier, &e);
}

static void evaluate_statement_fun_decl(Statement* stmt) {
  Value fn = value_new_fun(stmt->fun_decl.body, stmt->fun_decl.params.xs, stmt->fun_decl.params.count, scope_get_ref());
  scope_insert(stmt->fun_decl.identifier, &fn);
}

static void evaluate_statement_block(Statement* stmt) {
  for (size_t i = 0; i < stmt->block.count; ++i) {
    Statement* s = stmt->block.xs + i;
    evaluate_statement(s);   

    if (should_return())
      break;
  }
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
      goto cleanup;
    }

    if(e.bvalue) {
      evaluate_statement(b->branch);
      goto cleanup;
    }

  cleanup:
    value_scopeexit(&e);
    return;
  }
}

static void evaluate_statement_while(Statement* stmt) {
  Value e = evaluate_expression(stmt->while_loop.condition);

  if (!convert_to(&e, EVAL_TYPE_BOOL)) {
    runtime_error(NULL, "While loop condition does not evaluate to bool");
    goto cleanup;
  }

  bool iterate = e.bvalue;
  while (iterate) {
    evaluate_statement(stmt->while_loop.body);

    value_scopeexit(&e);
    e = evaluate_expression(stmt->while_loop.condition);
    
    if (!convert_to(&e, EVAL_TYPE_BOOL)) {
      runtime_error(NULL, "While loop condition does not evaluate to bool");
      goto cleanup;
    }

    iterate = e.bvalue;
  }

cleanup:
  value_scopeexit(&e);
  return;
}

static void evaluate_statement_return(Statement* stmt) {
  if (pending_return.await_return) {
    set_return(evaluate_expression(stmt->ret));
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
      scope_new();
      evaluate_statement_block(stmt);
      scope_pop();
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
      printf("Function(");
      for (size_t i = 0; i < e->fnvalue.params.count; ++i) {
        printf(SV_Fmt, SV_Fmt_arg(e->fnvalue.params.xs[i]));
      }
      printf(")\n");
    break;
    case EVAL_TYPE_NIL:
      printf("NIL\n");
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
