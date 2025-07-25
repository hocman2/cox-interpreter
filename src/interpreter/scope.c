#include "scope.h"

#include <stdlib.h>
#include <string.h>

#include "../types/arena.h"

static struct Scopes scopes = {NULL, 0, 0};

Scope* scope_new(Scope* upper) {
  const size_t scopes_initial_cap = 5;
  const size_t id_initial_cap = 10;

  if (scopes.scopes == NULL) {
    scopes.scopes = malloc(sizeof(Scope) * scopes_initial_cap);
    scopes.num_scopes = 0;
    scopes._cap = scopes_initial_cap;
  }

  if (scopes.num_scopes == scopes._cap) {
    scopes._cap *= 2;
    scopes.scopes = realloc(scopes.scopes, sizeof(Scope) * scopes._cap);
  }

  Scope* s = scopes.scopes + scopes.num_scopes;
  s->upper = upper;
  s->identifiers = malloc(sizeof(Identifier) * id_initial_cap);
  s->num_identifiers = 0;
  s->_cap = id_initial_cap;

  scopes.num_scopes += 1;

  return s;
}

void scope_insert(Scope* s, StringView name, const Evaluation* value) {
  // Update existing identifier if it already exists
  // as specified by lang spec
  Identifier* id_maybe = scope_get_ident(s, name);
  if (id_maybe) {
    id_maybe->value = *value;
    return;
  }

  Identifier id = {
    .name = name,
    .value = *value,
  };

  size_t cap = s->_cap;
  if (s->num_identifiers == cap) {
    s->_cap = cap * 2;
    s->identifiers = realloc(s->identifiers, sizeof(Identifier) * s->_cap); 
  }

  s->identifiers[s->num_identifiers] = id;
  s->num_identifiers += 1;
}

bool scope_replace(Scope* s, StringView name, const Evaluation* value) {
  Identifier* id_maybe = scope_get_ident(s, name);
  if (!id_maybe) {
    if (!s->upper) return false;
    return scope_replace(s->upper, name, value);
  }

  id_maybe->value = *value;
  return true;
}

Identifier* scope_pop(Scope* s) {
  if (s->num_identifiers == 0) return NULL;
  Identifier* id = &(s->identifiers[s->num_identifiers]);
  s->num_identifiers -= 1;
  return id;
}

Identifier* scope_get_ident(Scope* s, StringView name) {
  for (size_t i = 0; i < s->num_identifiers; ++i) {
    Identifier* id = s->identifiers + i;
    if (strncmp(id->name.str, name.str, name.len) == 0)
      return id;
  }

  return NULL;
}

static Identifier* _scope_get_ident_recursive(Scope* s, StringView name) {
  Identifier* id = scope_get_ident(s, name);

  if (0);
  else if (id) return id;
  else if (s->upper) return _scope_get_ident_recursive(s->upper, name);
  else return NULL;
}

Evaluation* scope_get_val(Scope* s, StringView name) {
  Identifier* id = _scope_get_ident_recursive(s, name);
  if (id) return &(id->value);
  else return NULL;
}

void scope_free(Scope* s) {
  free(s->identifiers);
}
