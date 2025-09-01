#ifndef _REF_COUNT_H
#define _REF_COUNT_H

#define MAX_RC_BLOCKS 20

#include <stdlib.h>

typedef struct {
  uint64_t id;
  uint64_t count;
  void* held_rsc;
  void (*free_fn)(void*);
} RcBlock;

struct RcBlockAlloc {
  RcBlock block;
  bool in_use;
};

extern struct RcBlockAlloc rc_blocks[MAX_RC_BLOCKS];

#define rc_null(ref) do { \
  if ((ref)->rc) { \
    rc_release((ref)); \
  } else { \
    (ref)->rsc = NULL; \
    (ref)->rc = NULL; \
  } \
} while (0)

#define rc_new(r, free_fn, outref) do { \
  (outref)->rc = _rc_new_impl(r, free_fn); \
  (outref)->rsc = (r); \
} while (0)

#define rc_acquire(other, outnew) do { \
   _rc_acquire_impl((other).rc); \
  (outnew)->rc = (other).rc; \
  (outnew)->rsc = (other).rsc; \
} while (0)

#define rc_release(ref) do { \
  uint64_t count_before = (ref)->rc->count; \
  bool free = _rc_release_impl((ref)->rsc, (ref)->rc); \
  uint64_t count_after = (ref)->rc->count; \
  (ref)->rc = NULL; \
  (ref)->rsc = NULL; \
} while (0)

#define rc_move(dst, src) do { \
  (dst)->rc = (src)->rc; \
  (dst)->rsc = (src)->rsc; \
  (src)->rc = NULL; \
  (src)->rsc = NULL; \
} while (0)

RcBlock* _rc_new_impl(void* held_rsc, void (*free_fn)(void*));
void _rc_acquire_impl(RcBlock* rc);
bool _rc_release_impl(void* rsc, RcBlock* rc);

#include <stdio.h>
static void print_rc_blocks() {
  int in_use_ct = 0;
  for (size_t i = 0; i < MAX_RC_BLOCKS; ++i) {
    if (rc_blocks[i].in_use) {
      in_use_ct += 1;
      }
  }

  printf("%d in use\n", in_use_ct);
}
#endif

