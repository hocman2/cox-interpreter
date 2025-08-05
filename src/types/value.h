#ifndef _VALUE_H
#define _VALUE_H

#include "string_view.h"
#include "../error/analysis.h"
#include "statements.h"

enum ValueType {
  EVAL_TYPE_DOUBLE,
  EVAL_TYPE_STRING_VIEW,
  EVAL_TYPE_BOOL,
  EVAL_TYPE_ERR,
  EVAL_TYPE_FUN,
};

struct FunctionParameters {
  StringView* xs;
  size_t count;
  size_t capacity;
};

struct FunctionValue {
  struct FunctionParameters params;
  Statements body;
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
  }
}
bool is_convertible_to_type(const Value* e, enum ValueType expected); 
bool convert_to(Value* e, enum ValueType to_type); 

#endif
