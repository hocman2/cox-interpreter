#ifndef _SCOPE_REF_H
#define _SCOPE_REF_H
#include "../types/ref_count.h"

typedef struct {
  struct Scope* rsc;
  RcBlock* rc;
} ScopeRef;

#endif
