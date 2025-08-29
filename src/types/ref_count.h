#ifndef _REF_COUNT_H
#define _REF_COUNT_H

#define MAX_RC_BLOCKS 20

#include <stdlib.h>

typedef struct {
  uint64_t count;
  void (*free_fn)(void*);
} RcBlock;

struct RcBlockAlloc {
  RcBlock block;
  bool in_use;
};

extern struct RcBlockAlloc rc_blocks[MAX_RC_BLOCKS];

#define rc_new(r, free_fn, outref) do { \
  (outref)->rc = _rc_new_impl(free_fn); \
  (outref)->rsc = (r); \
} while (0)

#define rc_acquire(other, outnew) do { \
   _rc_acquire_impl((other).rc); \
  (outnew)->rc = (other).rc; \
  (outnew)->rsc = (other).rsc; \
} while (0)

#define rc_release(ref) do { \
  _rc_release_impl((ref)->rsc, (ref)->rc); \
  (ref)->rc = NULL; \
  (ref)->rsc = NULL; \
} while (0)

#define rc_move(dst, src) do { \
  (dst)->rc = (src)->rc; \
  (dst)->rsc = (src)->rsc; \
  (src)->rc = NULL; \
  (src)->rsc = NULL; \
} while (0)

#define rc_null(ref) do { \
  if ((ref)->rc) { \
    rc_release(ref); \
  } else { \
    (ref)->rsc = NULL; \
  } \
} while (0)

RcBlock* _rc_new_impl(void (*free_fn)(void*));
void _rc_acquire_impl(RcBlock* rc);
void _rc_release_impl(void* rsc, RcBlock* rc);

#endif

