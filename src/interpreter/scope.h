#ifndef _SCOPE_H
#define _SCOPE_H

#include <stdint.h>
#include "../types/string_view.h"
#include "../types/value.h"
#include "../types/vector.h"

typedef struct {
  StringView name;
  Value value;
} StoredValue;

typedef struct Scope Scope;
struct Scope {
  Scope* upper;
  size_t capacity;
  size_t count;
  StoredValue* xs;
};

void scope_new();
void scope_insert(StringView name, const Value* value);
bool scope_replace(StringView name, const Value* value);
Value* scope_get_val(StringView name);
void scope_pop();

#endif
