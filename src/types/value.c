#include "value.h"
#include "ref_count.h"
#include "allocators/pool.h"
#include "statements.h"
#include "../error/runtime.h"
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
      printf("Function[%llu](", e->fnvalue.capture.rsc->id);
      for (size_t i = 0; i < e->fnvalue.params.count; ++i) {
        printf(SV_Fmt, SV_Fmt_arg(e->fnvalue.params.xs[i]));
      }
      printf(")");
    break;
    case EVAL_TYPE_CLASS:
      printf("Class: "SV_Fmt, SV_Fmt_arg(e->classvalue.rsc->name));
    break;
    case EVAL_TYPE_INSTANCE:
      printf("Instance of "SV_Fmt, SV_Fmt_arg(e->instancevalue.rsc->class.rsc->name));
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

static Pool class_pool; 
static Pool instance_pool; 
static StringView this_kw;

void value_init(size_t num_classes, size_t num_instances, StringView this_keyword) {
  pool_new(&class_pool, sizeof(struct ClassValue), num_classes);
  pool_new(&instance_pool, sizeof(struct InstanceValue), num_instances);
  this_kw = this_keyword;
}

void value_free() {
  pool_freeall(&class_pool);
  pool_freeall(&instance_pool);
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
  e.fnvalue = (FunctionValue){
    .params = {0},
    .body = body,
  };

  rc_move(&(e.fnvalue.capture), &capture);

  vector_new(e.fnvalue.params, num_params);
  for (size_t i = 0; i < num_params; ++i) {
    vector_push(e.fnvalue.params, params[i]);
  }

  return e; 
}

void class_free(void* rsc) {
  struct ClassValue* class = (struct ClassValue*)rsc;
  for (size_t i = 0; i < class->methods.count; ++i) {
    ClassMethod* method = class->methods.xs + i;
    rc_release(&method->method.fnvalue.capture);
  }
  vector_free(class->methods);
  pool_free(&class_pool, rsc);
}

Value value_new_class(StringView name, ClassMethods methods) {
  struct ClassValue* class; 
  pool_alloc(&class_pool, (void**)&class);
  class->name = name;
  vector_new(class->methods, methods.count);
  for (size_t i = 0; i < methods.count; ++i) {
    const ClassMethod* m = methods.xs + i;
    assert(m->method.type == EVAL_TYPE_FUN && "Method value is not a function type");
    ClassMethod method = {m->identifier, m->method};
    vector_push(class->methods, method);
  }

  ClassRef classref;
  rc_new(class, class_free, &classref);

  Value e;
  e.type = EVAL_TYPE_CLASS;
  rc_move(&(e.classvalue), &classref); 

  return e;
}

void instance_free(void* rsc) {
  struct InstanceValue* inst = (struct InstanceValue*)rsc;

  for (size_t i = 0; i < inst->properties.count; ++i) {
    struct InstanceProperty* prop = inst->properties.xs + i;
    value_scopeexit(&prop->value);
  }
  vector_free(inst->properties);

  rc_release(&inst->class);
  pool_free(&instance_pool, rsc);
}

Value value_new_instance(Value* class) {
  assert(class->type == EVAL_TYPE_CLASS && "Attempted to instanciate a class but passed value is not a Class");
  
  struct InstanceValue* instance;
  pool_alloc(&instance_pool, (void**)&instance);
  rc_acquire(class->classvalue, &(instance->class));
  vector_new(instance->properties, 1);

  Value e;
  e.type = EVAL_TYPE_INSTANCE;
  rc_new(instance, instance_free, &e.instancevalue); 

  return e;
}

Value instance_method_set_capture(const Value* fun, ScopeRef new_capture) {
  Value new;
  new.type = EVAL_TYPE_FUN;
  new.fnvalue = (FunctionValue) {
    .params = fun->fnvalue.params,
    .body = fun->fnvalue.body,
  };

  rc_acquire(new_capture, &new.fnvalue.capture);

  return new;
}


Value instance_find_property(const Value* instance, StringView name) {
#ifdef _DEBUG
  assert(instance != NULL && "Attempted to find property on NULL instance");
  assert(instance->type == EVAL_TYPE_INSTANCE && "Attempted to find property on non-instance");
#endif

  for (size_t i = 0; i < instance->instancevalue.rsc->properties.count; ++i) {
    const struct InstanceProperty* prop = 
      instance->instancevalue.rsc->properties.xs + i;

    if (strncmp(prop->identifier.str, name.str, prop->identifier.len) == 0) {
      return value_copy(&prop->value);
    }
  }

  // Look for method
  struct ClassValue* class = instance->instancevalue.rsc->class.rsc;
  for (size_t i = 0; i < class->methods.count; ++i) {
    ClassMethod* method = class->methods.xs + i;

    if (strncmp(method->identifier.str, name.str, method->identifier.len) == 0) {
      ScopeRef instance_capture = scope_create();
      scope_insert_into(instance_capture, this_kw, instance);
      Value ret = instance_method_set_capture(&method->method, instance_capture);
      rc_release(&instance_capture);
      return ret;
    }
  }

  return value_new_nil();
}

void instance_set_property(Value* instance, StringView name, const Value* insert) {
  for (size_t i = 0; i < instance->instancevalue.rsc->properties.count; ++i) {
    struct InstanceProperty* prop = instance->instancevalue.rsc->properties.xs + i;
    if (strncmp(prop->identifier.str, name.str, prop->identifier.len) == 0) {
      prop->value = value_copy(insert);
      return;
    }
  }

  struct InstanceProperty new_prop = {
    .identifier = name,
    .value = value_copy(insert),
  };

  vector_push(instance->instancevalue.rsc->properties, new_prop);
}

Value value_copy(const Value* v) {
  switch (v->type) {
    case EVAL_TYPE_FUN: {
      Value fn = *v;
      fn.fnvalue.capture = scope_ref_acquire(v->fnvalue.capture);
      return fn;
    }
    break;
    case EVAL_TYPE_CLASS: {
      Value class = *v;
      rc_acquire(v->classvalue, &(class.classvalue));
      return class;
    }
    break;
    case EVAL_TYPE_INSTANCE: {
      Value instance = *v;
      rc_acquire(v->instancevalue, &instance.instancevalue);
      return instance;
    }
    break;
    default:
    return *v;
  }
}

ClassMethods build_class_methods(struct ClassMethodsDecl methods_decl) {
  ClassMethods methods;
  vector_new(methods, methods_decl.count);

  for (size_t i = 0; i < methods_decl.count; ++i) {
    StatementMethodDecl* method = methods_decl.xs + i;
    
    Value fn = value_new_fun(method->body, method->params.xs, method->params.count, scope_ref_get_current());

    ClassMethod built_method = {method->identifier, fn};
    vector_push(methods, built_method);
  }

  return methods;
}

void value_scopeexit(Value* v) {
  switch (v->type) {
    case EVAL_TYPE_FUN: {
      rc_release(&v->fnvalue.capture);
    }
    break;
    case EVAL_TYPE_CLASS:
      rc_release(&v->classvalue);
    break;
    case EVAL_TYPE_INSTANCE:
      rc_release(&v->instancevalue);
    break;
    default: 
    break;
  }
}
