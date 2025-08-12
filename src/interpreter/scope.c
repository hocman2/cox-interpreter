#include "scope.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../types/arena.h"
#include "../types/vector.h"
#include "../types/ref_count.h"
#include "scope_ref.h"

#define MAX_SCOPES 256

static Arena scope_alloc;
static ScopeRef curr_scope = NULL;
static ScopeRef sided_scope = NULL;

ScopeRef scope_get_ref() { 
  ref_count_incr(*curr_scope);
  return curr_scope;
}

ScopeRef scope_new_ref(Scope* s) {
  ref_count_incr(*s);
  return s;
}

// Same as below but the name makes the usage more clear
ScopeRef scope_copy_ref(ScopeRef s) {
  ref_count_incr(*s);
  return s;
}

void scope_release_ref(ScopeRef ref) {
  ref_count_decr(*ref);
}

void scope_new() {
  const size_t ID_INITIAL_CAP = 10;

  static bool scopes_init = false;
  if (!scopes_init) {
    scope_alloc = arena_init(sizeof(Scope) * MAX_SCOPES, alignof(Scope));
    scopes_init = true;
  }

  Scope* new = arena_alloc(&scope_alloc, sizeof(Scope));
  ref_count_new(*new, scope_free);
  vector_new(*new, ID_INITIAL_CAP); 

  ScopeRef new_ref = scope_new_ref(new);

  if (curr_scope) {
    new_ref->upper = scope_copy_ref(curr_scope);
    scope_release_ref(curr_scope);
  }

  // Same as curr_scope = new_ref technically
  curr_scope = scope_copy_ref(new_ref);
  scope_release_ref(new_ref);
}

void scope_swap(ScopeRef new) {
  if (!curr_scope) assert(false && "Attempted to scope swap with NULL curr_scope");
  if (!new) assert(false && "Attempted to scope swap with NULL new scope");

  sided_scope = scope_copy_ref(curr_scope);
  scope_release_ref(curr_scope);

  curr_scope = scope_copy_ref(new);
}

void scope_restore() {
  curr_scope = scope_copy_ref(sided_scope);
  scope_release_ref(sided_scope);
  sided_scope = NULL;
}

static StoredValue* _scope_get_ident_recursive(Scope* s, StringView name) {
  StoredValue* id = NULL;

  for (size_t i = 0; i < s->count; ++i) {
    StoredValue* el = s->xs + i;
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

void scope_insert(StringView name, const Value* value) {
  Scope* s = curr_scope;
  // Update existing identifier if it already exists
  // as specified by lang spec
  StoredValue* id_maybe = _scope_get_ident_recursive(s, name);
  if (id_maybe) {
    id_maybe->value = *value;
    return;
  }

  StoredValue id = {
    .name = name,
    .value = *value,
  };

  vector_push((*s), id);
}

bool scope_replace(StringView name, const Value* value) {
  Scope* s = curr_scope;
  StoredValue* id_maybe = _scope_get_ident_recursive(s, name);

  if (!id_maybe) {
    return false;
  }

  id_maybe->value = *value;
  return true;
}


ValueRef scope_get_val_ref(StringView name) {
  Scope* s = curr_scope;
  StoredValue* id = _scope_get_ident_recursive(s, name);
  if (id) return &(id->value);
  else return NULL;
}

Value scope_get_val_copy(StringView name) {
  Scope* s = curr_scope;
  StoredValue* id = _scope_get_ident_recursive(s, name);
  if (id) return value_copy(&id->value);
  else return value_new_err();
}

void scope_pop() {
  for (size_t i = 0; i < curr_scope->count; ++i) {
    StoredValue* v = curr_scope->xs + i;
    value_scopeexit(&v->value);
  }

  if (!curr_scope->upper) {
    // Global scope is popped, force the free
    arena_free(&scope_alloc);
    scope_free(curr_scope);
    curr_scope = NULL;
  } else {
    ScopeRef upper = scope_copy_ref(curr_scope->upper);
    scope_release_ref(curr_scope);

    // We can be memory economic and reuse the memory location on next scope_new
    if (curr_scope->ref_count == 0) {
      arena_pop(&scope_alloc);
    }

    curr_scope = scope_copy_ref(upper);
    scope_release_ref(upper);
  }
}

void scope_free(Scope* s) {
  if (!s) return;
  vector_free(*s);
}
