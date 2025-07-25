#ifndef _EVALUATION_H
#define _EVALUATION_H

#include "string_view.h"

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

#endif
