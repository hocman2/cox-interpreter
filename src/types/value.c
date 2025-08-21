#include "value.h"
#include "ref_count.h"
#include "../interpreter/scope.h"

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

void value_pretty_print(const Value* e) {
  switch(e->type) {
    case EVAL_TYPE_DOUBLE:
      printf("Double: %f", e->dvalue);
    break;
    case EVAL_TYPE_STRING_VIEW:
      printf("String: "SV_Fmt, SV_Fmt_arg(e->svvalue));
    break;
    case EVAL_TYPE_BOOL:
      printf("Boolean: %s", e->bvalue ? "true" : "false");
    break;
    case EVAL_TYPE_FUN:
      printf("Function[%llu](", e->fnvalue.capture->id);
      for (size_t i = 0; i < e->fnvalue.params.count; ++i) {
        printf(SV_Fmt, SV_Fmt_arg(e->fnvalue.params.xs[i]));
      }
      printf(")");
    break;
    case EVAL_TYPE_CLASS:
      printf("Class: "SV_Fmt, SV_Fmt_arg(e->classvalue.name));
    break;
    case EVAL_TYPE_NIL:
      printf("NIL");
    break;
    case EVAL_TYPE_ERR:
      printf("Error");
    break;
  }
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

Value value_new_double(double val) {
  Value e;
  e.type = EVAL_TYPE_DOUBLE;
  e.dvalue = val;
  return e;
}

Value value_new_stringview(StringView sv) {
  Value e;
  e.type = EVAL_TYPE_STRING_VIEW;
  e.svvalue = sv;
  return e;
}

Value value_new_bool(bool val) {
  Value e;
  e.type = EVAL_TYPE_BOOL;
  e.bvalue = val;
  return e;
}

Value value_new_err() {
  return (Value){EVAL_TYPE_ERR};
}

Value value_new_nil() {
  return (Value){EVAL_TYPE_NIL};
}

Value value_new_fun(Statement* body, const StringView* params, size_t num_params, ScopeRef capture) {
  assert(body->type == STATEMENT_BLOCK && "Attempted to create a function value with non block body");

  Value e;
  e.type = EVAL_TYPE_FUN;
  e.fnvalue = (struct FunctionValue){
    .params = {0},
    .body = body,
    .capture = scope_ref_move(capture),
  };

  vector_new(e.fnvalue.params, num_params);
  for (size_t i = 0; i < num_params; ++i) {
    vector_push(e.fnvalue.params, params[i]);
  }

  return e; 
}

Value value_new_class(StringView name) {
  Value e;
  e.type = EVAL_TYPE_CLASS;
  e.classvalue = (struct ClassValue) {
    .name = name,
  };

  return e;
}

Value value_copy(const Value* v) {
  switch (v->type) {
    case EVAL_TYPE_FUN: {
      Value fn = *v;
      fn.fnvalue.capture = scope_ref_acquire(v->fnvalue.capture);
      return fn;
    }
    break;
    default:
    return *v;
  }
}

void value_scopeexit(Value* v) {
  switch (v->type) {
    case EVAL_TYPE_FUN:
      scope_ref_release(v->fnvalue.capture);
    break;
    default: 
    break;
  }
}
