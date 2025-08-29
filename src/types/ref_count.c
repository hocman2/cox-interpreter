#include "ref_count.h"
#ifdef _DEBUG
#include <assert.h>
#endif

struct RcBlockAlloc rc_blocks[MAX_RC_BLOCKS] = {0};

RcBlock* _rc_new_impl(void (*free_fn)(void*)) {
  RcBlock* block = NULL;
  for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
    if (rc_blocks[i].in_use) continue;
    block = &rc_blocks[i].block;
  }

#ifdef _DEBUG
  assert(block && "Max ref count blocks allocated");
#endif

  block->free_fn = free_fn;
  block->count = 1;
  return block;
}

void _rc_acquire_impl(RcBlock* rc) {
#ifdef _DEBUG
  assert(rc != NULL && "Tried acquiring a NULL rc block");
#endif
  rc->count += 1;
}

void _rc_release_impl(void* rsc, RcBlock* rc) {
#ifdef _DEBUG
  assert(rc != NULL && "Tried releasing a NULL rc block");
  assert(rc->count > 0 && "Tried releasing an already freed rc block");
#endif

  rc->count -= 1;
  if (rc->count == 0) {
    rc->free_fn(rsc);
    for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
      if (&rc_blocks[i].block == rc) {
        rc_blocks[i].in_use = false;
      }
    }
  }
}
