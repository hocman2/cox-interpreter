#include "ref_count.h"
#ifdef _DEBUG
#include <assert.h>
#endif

RcBlock rc_blocks[MAX_RC_BLOCKS] = {0};
size_t num_rc_blocks = 0;

RcBlock* _rc_new_impl(void (*free_fn)(void*)) {
#ifdef _DEBUG
  assert(num_rc_blocks < MAX_RC_BLOCKS && "Max Ref count blocks allocated");
#endif

  RcBlock* block = rc_blocks + num_rc_blocks;
  num_rc_blocks += 1;

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
  }
}
