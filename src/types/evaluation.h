#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "string_view.h"
#include "../error/analysis.h"

enum EvalType {
  EVAL_TYPE_DOUBLE,
  EVAL_TYPE_STRING_VIEW,
  EVAL_TYPE_BOOL,
  EVAL_TYPE_ERR,
};

typedef struct {
  enum EvalType type;
  union {
    double dvalue;
    StringView svvalue;
    bool bvalue;
  };
} Evaluation;

static const char* eval_type_to_str(enum EvalType t) {
  switch (t) {
  case EVAL_TYPE_DOUBLE:
    return "double";
  case EVAL_TYPE_STRING_VIEW:
    return "string";
  case EVAL_TYPE_BOOL:
    return "bool";
  case EVAL_TYPE_ERR:
    return "err";
  }
}
bool is_convertible_to_type(const Evaluation* e, enum EvalType expected); 
bool convert_to(Evaluation* e, enum EvalType to_type); 

#endif
