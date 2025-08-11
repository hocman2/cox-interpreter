#ifndef _VALUE_H
#define _VALUE_H

#include "string_view.h"
#include "../error/analysis.h"
#include "statements.h"
#include "../interpreter/scope_ref.h"

enum ValueType {
  EVAL_TYPE_DOUBLE,
  EVAL_TYPE_STRING_VIEW,
  EVAL_TYPE_BOOL,
  EVAL_TYPE_ERR,
  EVAL_TYPE_FUN,
  EVAL_TYPE_NIL,
};

struct FunctionParameters {
  StringView* xs;
  size_t count;
  size_t capacity;
};

struct FunctionValue {
  struct FunctionParameters params;
  Statement* body;
  ScopeRef capture;
};

typedef struct {
  enum ValueType type;
  union {
    double dvalue;
    StringView svvalue;
    bool bvalue;
    struct FunctionValue fnvalue;
  };
} Value;

typedef Value* ValueRef;

static const char* eval_type_to_str(enum ValueType t) {
  switch (t) {
  case EVAL_TYPE_DOUBLE:
    return "double";
  case EVAL_TYPE_STRING_VIEW:
    return "string";
  case EVAL_TYPE_BOOL:
    return "bool";
  case EVAL_TYPE_ERR:
    return "err";
  case EVAL_TYPE_FUN:
    return "fun";
  case EVAL_TYPE_NIL:
    return "nil";
  }
}

bool is_convertible_to_type(const Value* e, enum ValueType expected); 
bool convert_to(Value* e, enum ValueType to_type); 

Value value_new_double(double val);
Value value_new_stringview(StringView sv);
Value value_new_bool(bool val);
Value value_new_err();
Value value_new_nil();
Value value_new_fun(const struct StatementFunDecl* fundecl, ScopeRef capture);

Value value_copy(Value* v);
void value_scopeexit(Value* v);

#endif
