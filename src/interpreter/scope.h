#ifndef _SCOPE_H
#define _SCOPE_H

#include <stdint.h>
#include "../types/string_view.h"
#include "../types/evaluation.h"
#include "../types/vector.h"

typedef struct {
  StringView name;
  Evaluation value;
} Identifier;

typedef struct Scope Scope;
struct Scope {
  Scope* upper;
  Vector identifiers;
};

void scope_new();
void scope_insert(StringView name, const Evaluation* value);
bool scope_replace(StringView name, const Evaluation* value);
Evaluation* scope_get_val(StringView name);
void scope_pop();

#endif
