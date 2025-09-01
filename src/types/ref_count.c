#include "ref_count.h"
#ifdef _DEBUG
#include <assert.h>
#endif

struct RcBlockAlloc rc_blocks[MAX_RC_BLOCKS];
static uint64_t block_id_gen = 0;

RcBlock* _rc_new_impl(void* held_rsc, void (*free_fn)(void*)) {
  static bool rc_blocks_init = false;
  if (!rc_blocks_init) {
    for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
      rc_blocks[i].block = (RcBlock){block_id_gen++, 0, NULL, NULL};
      rc_blocks[i].in_use = false;
    }
    rc_blocks_init = true;
  }

  RcBlock* block = NULL;
  for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
    if (!rc_blocks[i].in_use) {
      rc_blocks[i].in_use = true;
      block = &rc_blocks[i].block;
      block->id = block_id_gen++;
      break;
    }
  }

#ifdef _DEBUG
  assert(block && "Max ref count blocks allocated");
#endif

  block->held_rsc = held_rsc;
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

bool _rc_release_impl(void* rsc, RcBlock* rc) {
#ifdef _DEBUG
  assert(rc != NULL && "Tried releasing a NULL rc block");
  assert(rc->count > 0 && "Tried releasing an already freed rc block");
#endif

  bool ret = false;

  rc->count -= 1;
  if (rc->count == 0) {
    rc->free_fn(rsc);
    ret = true;
    for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
      if (&rc_blocks[i].block == rc) {
        rc_blocks[i].in_use = false;
        break;
      }
    }
  }

  return ret;
}
