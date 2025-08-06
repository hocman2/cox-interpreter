#include "value.h"

// All of this complex machinery allows us to define type conversions rather easily

// This macro is used
#define DEFINE_CONVERT_FN(NAME, OUT_TYPE) \
const static struct ConvertionFunction CONVERT_##NAME = {EVAL_TYPE_##OUT_TYPE, NAME##_fn};

struct ConvertionFunction {
  enum ValueType out_type;
  void (*fn)(Value* e);
};

struct ConvertionTableField {
  enum ValueType in_type;
  struct ConvertionFunction convertible_to[10];
};

// CONVERTION FUNCTIONS //
// To define a conversion function do the following:
// 1. Write a function with capitalized name INTYPE_TO_OUTTYPE(Value* e)
// 2. Then call the DEFINE_CONVERT_FN macro to generate a 'struct ConvertionFunction' object automatically
// 3. In the convertion_table below, you can use the CONVERT_INTYPE_TO_OUTTYPE generated struct as a convertion function

void DOUBLE_TO_BOOL_fn(Value *e) {
  double val = e->dvalue;
  e->type = EVAL_TYPE_BOOL;
  e->bvalue = val == 0.0;
}
DEFINE_CONVERT_FN(DOUBLE_TO_BOOL, BOOL);

void BOOL_TO_DOUBLE_fn(Value* e) {
  bool val = e->bvalue;
  e->type = EVAL_TYPE_DOUBLE;
  e->dvalue = (val) ? 1.0 : 0.0;
}
DEFINE_CONVERT_FN(BOOL_TO_DOUBLE, DOUBLE);

void NIL_TO_BOOL_fn(Value* e) {
  e->type = EVAL_TYPE_BOOL;
  e->bvalue = false;
}
DEFINE_CONVERT_FN(NIL_TO_BOOL, BOOL);

static const struct ConvertionFunction CONVERT_STOP = {0, NULL};

// END CONVERTION FUNCTIONS //

static const struct ConvertionTableField convertion_table[] = {
  {EVAL_TYPE_DOUBLE,  {CONVERT_DOUBLE_TO_BOOL, CONVERT_STOP}},
  {EVAL_TYPE_BOOL,    {CONVERT_BOOL_TO_DOUBLE, CONVERT_STOP}},
  {EVAL_TYPE_NIL,     {CONVERT_NIL_TO_BOOL, CONVERT_STOP}},
  {0, {CONVERT_STOP}},
};


bool is_convertible_to_type(const Value* e, enum ValueType expected) {
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

bool convert_to(Value* e, enum ValueType to_type) {
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
