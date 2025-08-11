#ifndef _REF_COUNT_H
#define _REF_COUNT_H

// Ref counting interface for struct
// Must have the following fields:
// [int, long, size_t, etc.] ref_count: actual counter
// void (*ref_count_free)([type]*): the free function to call after a decrement leading to a 0 ref_count

#define ref_count_new(x, freefn) \
do { \
  (x).ref_count = 0; \
  (x).ref_count_free = freefn; \
} while (0)

#define ref_count_incr(x) \
do { \
  (x).ref_count += 1; \
} while (0)

#define ref_count_decr(x) \
do { \
  assert((x).ref_count > 0 && "Ref counted pointer was removed more than it was added !\n"); \
  (x).ref_count -= 1; \
  if ((x).ref_count == 0) (x).ref_count_free(&(x)); \
} while (0)

#endif
