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
  EVAL_TYPE_CLASS,
  EVAL_TYPE_INSTANCE,
  EVAL_TYPE_NIL,
};

struct FunctionParameters {
  StringView* xs;
  size_t count;
  size_t capacity;
};

typedef struct {
  struct FunctionParameters params;
  Statement* body;
  ScopeRef capture;
} FunctionValue;

typedef struct {
  struct ClassValue* rsc;
  RcBlock* rc;
} ClassRef;

typedef struct {
  RcBlock* rc;
  struct InstanceValue* rsc;
} InstanceRef;

typedef struct {
  enum ValueType type;
  union {
    double dvalue;
    StringView svvalue;
    bool bvalue;
    FunctionValue fnvalue;
    ClassRef classvalue;
    InstanceRef instancevalue;
  };
} Value;

typedef struct {
  StringView identifier;
  Value method;
} ClassMethod;

typedef struct {
  ClassMethod* xs;
  size_t count;
  size_t capacity;
} ClassMethods;

struct ClassValue {
  StringView name;
  ClassMethods methods;
};

struct InstanceProperty {
  StringView identifier;
  Value value;
};

struct InstanceProperties {
  struct InstanceProperty* xs;
  size_t count;
  size_t capacity;
};

struct InstanceValue {
  ClassRef class;
  struct InstanceProperties properties;
};

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
  case EVAL_TYPE_CLASS:
    return "class";
  case EVAL_TYPE_INSTANCE:
    return "instance";
  case EVAL_TYPE_NIL:
    return "nil";
  }
}

void value_pretty_print(const Value* v);
bool is_convertible_to_type(const Value* e, enum ValueType expected); 
bool convert_to(Value* e, enum ValueType to_type); 

void value_init(size_t num_classes, size_t num_instances, StringView this_keyword);
void value_free();
Value value_new_double(double val);
Value value_new_stringview(StringView sv);
Value value_new_bool(bool val);
Value value_new_err();
Value value_new_nil();
Value value_new_fun(Statement* body, const StringView* params, size_t num_params, ScopeRef capture);
ClassMethods build_class_methods(struct ClassMethodsDecl methods_decl);
Value value_new_class(StringView name, ClassMethods methods);
Value value_new_instance(Value* class);
Value instance_find_property(const Value* instance, StringView name);
void instance_set_property(Value* instance, StringView name, const Value* insert);

Value value_copy(const Value* v);
void value_scopeexit(Value* v);

#endif
