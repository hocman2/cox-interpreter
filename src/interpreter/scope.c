#include "scope.h"

#include <stdlib.h>
#include <string.h>

#include "../types/arena.h"
#include "../types/vector.h"

#define MAX_SCOPES 256

static Arena scope_alloc;
static Scope* curr_scope = NULL;

void scope_new() {
  const size_t ID_INITIAL_CAP = 10;

  static bool scopes_init = false;
  if (!scopes_init) {
    scope_alloc = arena_init(sizeof(Scope) * MAX_SCOPES, alignof(Scope));
    scopes_init = true;
  }

  Scope* upper_scope = curr_scope;
  curr_scope = arena_alloc(&scope_alloc, sizeof(Scope));
  curr_scope->upper = upper_scope;
  vector_new((*curr_scope), ID_INITIAL_CAP); 
}

static Identifier* _scope_get_ident_recursive(Scope* s, StringView name) {
  Identifier* id = NULL;

  for (size_t i = 0; i < s->count; ++i) {
    Identifier* el = s->xs + i;
    if (el->name.len == name.len) {
      if (strncmp(el->name.str, name.str, name.len) == 0) {
        id = el;
      }
    }
  }

  if (0);
  else if (id) return id;
  else if (s->upper) return _scope_get_ident_recursive(s->upper, name);
  else return NULL;
}

void scope_insert(StringView name, const Evaluation* value) {
  Scope* s = curr_scope;
  // Update existing identifier if it already exists
  // as specified by lang spec
  Identifier* id_maybe = _scope_get_ident_recursive(s, name);
  if (id_maybe) {
    id_maybe->value = *value;
    return;
  }

  Identifier id = {
    .name = name,
    .value = *value,
  };

  vector_push((*s), id);
}

bool scope_replace(StringView name, const Evaluation* value) {
  Scope* s = curr_scope;
  Identifier* id_maybe = _scope_get_ident_recursive(s, name);

  if (!id_maybe) {
    return false;
  }

  id_maybe->value = *value;
  return true;
}


Evaluation* scope_get_val(StringView name) {
  Scope* s = curr_scope;
  Identifier* id = _scope_get_ident_recursive(s, name);
  if (id) return &(id->value);
  else return NULL;
}

void scope_pop() {
  Scope* s = curr_scope;

  curr_scope = s->upper;
  vector_free((*s));
  arena_pop(&scope_alloc, sizeof(Arena));
}
