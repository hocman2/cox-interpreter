#include "evaluation.h"

#define DEFINE_CONVERT_FN(NAME, OUT_TYPE) \
const static struct ConvertionFunction CONVERT_##NAME = {EVAL_TYPE_##OUT_TYPE, NAME##_fn};

struct ConvertionFunction {
  enum EvalType out_type;
  void (*fn)(Evaluation* e);
};

struct ConvertionTableField {
  enum EvalType in_type;
  struct ConvertionFunction convertible_to[10];
};

// CONVERTION FUNCTIONS //

void DOUBLE_TO_BOOL_fn(Evaluation *e) {
  double val = e->dvalue;
  e->type = EVAL_TYPE_BOOL;
  e->bvalue = val == 0.0;
}
DEFINE_CONVERT_FN(DOUBLE_TO_BOOL, BOOL);

void BOOL_TO_DOUBLE_fn(Evaluation* e) {
  bool val = e->bvalue;
  e->type = EVAL_TYPE_DOUBLE;
  e->dvalue = (val) ? 1.0 : 0.0;
}
DEFINE_CONVERT_FN(BOOL_TO_DOUBLE, DOUBLE);

static const struct ConvertionFunction CONVERT_STOP = {0, NULL};

// END CONVERTION FUNCTIONS //

static const struct ConvertionTableField convertion_table[] = {
  {EVAL_TYPE_DOUBLE,  {CONVERT_DOUBLE_TO_BOOL, CONVERT_STOP}},
  {EVAL_TYPE_BOOL,    {CONVERT_BOOL_TO_DOUBLE, CONVERT_STOP}},
  {0, {CONVERT_STOP}},
};


bool is_convertible_to_type(const Evaluation* e, enum EvalType expected) {
  if (e->type == expected) {
    return true;
  }

  for (size_t i = 0; convertion_table[i].in_type != 0; ++i) {
    const struct ConvertionTableField* field = convertion_table + i;

    if (field->in_type == e->type) {
      for (size_t j = 0; field->convertible_to[j].fn != NULL; ++j) {
        const struct ConvertionFunction* convfn = field->convertible_to + j;
        if (convfn->out_type == expected) {
          return true;
        }
      }
    }
  }

  return false;
}

bool convert_to(Evaluation* e, enum EvalType to_type) {
  if (e->type == to_type) {
    return true;
  }

  for (size_t i = 0; convertion_table[i].in_type != 0; ++i) {
    const struct ConvertionTableField* field = convertion_table + i;

    if (field->in_type == e->type) {
      for (size_t j = 0; field->convertible_to[j].fn != NULL; ++j) {
        const struct ConvertionFunction* convfn = field->convertible_to + j;
        if (convfn->out_type == to_type) {
          convfn->fn(e);
          return true;
        }
      }
    }
  }

  return false;
}
