#include "scope.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../types/allocators/pool.h"
#include "../types/vector.h"
#include "../types/ref_count.h"
#include "scope_ref.h"

#define MAX_SCOPES 256

struct SidedScopes {
  ScopeRef* xs;
  size_t capacity;
  size_t count;
};

static uint64_t scope_ids = 0;
static Pool scope_alloc;
static ScopeRef curr_scope = {0};
static struct SidedScopes sided_scopes;

static void scope_stats(const Scope* s) {
  assert(s && "Can't print NULL scope");
  printf("Scope [%llu] (%zu values) %p", s->id, s->count, s);
  if (s->count) {
    printf(" - (");
    for (size_t i = 0; i < s->count; ++i) {
      StoredValue* v = s->xs + i;
      printf(SV_Fmt" -> ", SV_Fmt_arg(v->name));
      value_pretty_print(&v->value);
      if (i < s->count - 1) printf(", ");
    }
    printf(")\n");
  } else {
    printf("\n");
  }

  if (s->upper.rsc) {
    printf("-> ");
    scope_stats(s->upper.rsc);
  }
}

ScopeRef scope_ref_get_current() { 
  ScopeRef acq;
  rc_acquire(curr_scope, &acq);
  return acq;
}

void scope_new() {
  const size_t ID_INITIAL_CAP = 10;
  const size_t SIDED_INITIAL_CAP = 16;

  static bool scopes_init = false;
  if (!scopes_init) {
    pool_new(&scope_alloc, sizeof(Scope), MAX_SCOPES);
    vector_new(sided_scopes, SIDED_INITIAL_CAP);
    scopes_init = true;
  }

  Scope* newscope;
  pool_alloc(&scope_alloc, (void**)&newscope);
  newscope->id = scope_ids++;
  vector_new(*newscope, ID_INITIAL_CAP); 

  ScopeRef new_ref;
  rc_new(newscope, scope_free, &new_ref);

  if (curr_scope.rsc) {
    rc_move(&(new_ref.rsc->upper), &curr_scope);
  } else {
    rc_null(&(new_ref.rsc->upper));
  }

  rc_move(&curr_scope, &new_ref);
}

void scope_swap(ScopeRef new) {
  if (!curr_scope.rsc) assert(false && "Attempted to scope swap with NULL curr_scope");
  if (!new.rsc) assert(false && "Attempted to scope swap with NULL new scope");

  ScopeRef moved;
  rc_move(&moved, &curr_scope);
  vector_push(sided_scopes, moved);
  rc_acquire(new, &curr_scope);
}

void scope_restore() {
  assert(sided_scopes.count > 0 && "Attempted to scope_restore() without calling scope_swap first");
  ScopeRef popped;
  ScopeRef last_sided = sided_scopes.xs[sided_scopes.count - 1];
  rc_move(&popped, &last_sided);
  vector_pop(sided_scopes);

  rc_release(&curr_scope);
  rc_move(&curr_scope, &popped);
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
  else if (s->upper.rsc) return _scope_get_ident_recursive(s->upper.rsc, name);
  else return NULL;
}

static StoredValue* _scope_get_ident(Scope* s, StringView name) {
  StoredValue* id = NULL;

  for (size_t i = 0; i < s->count; ++i) {
    StoredValue* el = s->xs + i;
    if (el->name.len == name.len) {
      if (strncmp(el->name.str, name.str, name.len) == 0) {
        id = el;
      }
    }
  }

  if (id) return id;
  else return NULL;
}

static void scope_override_current() {
  Scope* newscope;
  pool_alloc(&scope_alloc, (void**)&newscope);
  vector_new(*newscope, curr_scope.rsc->count);
  newscope->id = scope_ids++;

  if (curr_scope.rsc->upper.rsc) {
    rc_acquire(curr_scope.rsc->upper, &(newscope->upper));
  } else {
    rc_null(&(newscope->upper));
  }

  for (size_t i = 0; i < curr_scope.rsc->count; ++i) {
    StoredValue* val = curr_scope.rsc->xs + i;
    vector_push(*newscope, ((StoredValue){val->name, value_copy(&val->value)}));
  }

  ScopeRef new_ref;
  rc_new(newscope, scope_free, &new_ref);

  ScopeRef old;
  rc_move(&old, &curr_scope);
  rc_move(&curr_scope, &new_ref);
  rc_release(&old);
}

void scope_insert(StringView name, const Value* value) {
  scope_override_current();

  Scope* s = (Scope*)curr_scope.rsc;
  // Update existing identifier if it already exists
  // as specified by lang spec
  StoredValue* id_maybe = _scope_get_ident(s, name);
  if (id_maybe) {
    id_maybe->value = value_copy(value);
    return;
  }

  StoredValue id = {
    .name = name,
    .value = value_copy(value),
  };

  vector_push((*s), id);
}

bool scope_replace(StringView name, const Value* value) {
  scope_override_current();

  Scope* s = (Scope*)curr_scope.rsc;
  StoredValue* id_maybe = _scope_get_ident_recursive(s, name);

  if (!id_maybe) {
    return false;
  }

  id_maybe->value = value_copy(value);
  return true;
}


ValueRef scope_get_val_ref(StringView name) {
  Scope* s = (Scope*)curr_scope.rsc;
  StoredValue* id = _scope_get_ident_recursive(s, name);
  if (id) return &(id->value);
  else return NULL;
}

Value scope_get_val_copy(StringView name) {
  Scope* s = (Scope*)curr_scope.rsc;
  StoredValue* id = _scope_get_ident_recursive(s, name);
  if (id) return value_copy(&id->value);
  else return value_new_err();
}

void scope_pop() {
  for (size_t i = 0; i < curr_scope.rsc->count; ++i) {
    StoredValue* v = curr_scope.rsc->xs + i;
    value_scopeexit(&v->value);
  }

  if (!curr_scope.rsc->upper.rsc) {
    // Global scope is popped, force the free
    rc_null(&curr_scope);
    scope_free((void*)curr_scope.rsc);
    pool_freeall(&scope_alloc);
  } else {
    ScopeRef upper;
    rc_acquire(curr_scope.rsc->upper, &upper);
    rc_release(&curr_scope);
    rc_move(&curr_scope, &upper);
  }
}

void scope_free(void* scope) {
  Scope* s = (Scope*)scope;
  if (!s) return;
  if (s->upper.rsc) rc_release(&s->upper);
  pool_free(&scope_alloc, s);
  vector_free(*s);
}
