#ifndef _SCOPE_H
#define _SCOPE_H

#include <stdint.h>
#include "../types/string_view.h"
#include "../types/evaluation.h"

typedef struct {
  StringView name;
  Evaluation value;
} Identifier;

typedef struct Scope Scope;
struct Scope {
  Scope* upper;
  Identifier* identifiers;
  size_t num_identifiers;
  size_t _cap;
};

struct Scopes {
  Scope* scopes;
  size_t num_scopes;
  size_t _cap;
};

Scope* scope_new(Scope* upper);
void scope_insert(Scope* scope, StringView name, const Evaluation* value);
bool scope_replace(Scope* scope, StringView name, const Evaluation* value);
Identifier* scope_pop(Scope* scope);
Evaluation* scope_get_val(Scope* scope, StringView name);
Identifier* scope_get_ident(Scope* scope, StringView name);
void scope_free(Scope* scope);

#endif
