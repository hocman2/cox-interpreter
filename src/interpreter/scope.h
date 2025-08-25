#ifndef _SCOPE_H
#define _SCOPE_H

#include <stdint.h>
#include "../types/string_view.h"
#include "../types/value.h"
#include "../types/vector.h"
#include "scope_ref.h"

typedef struct {
  StringView name;
  Value value;
} StoredValue;

typedef struct Scope Scope;
struct Scope {
  uint64_t id;
  ScopeRef upper;

  // dynamic array data for storedvalues, should be in its own datastructure
  size_t capacity;
  size_t count;
  StoredValue* xs;
};

ScopeRef scope_ref_get_current();
void scope_new();
void scope_swap(ScopeRef new);
void scope_insert(StringView name, const Value* value);
bool scope_replace(StringView name, const Value* value);
ValueRef scope_get_val_ref(StringView name);
Value scope_get_val_copy(StringView name);
void scope_pop();
void scope_restore();
void scope_free(void* scope);

#endif
