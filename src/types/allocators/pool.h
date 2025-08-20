#ifndef _POOL_H
#define _POOL_H

#include <stdlib.h>
#include <stdint.h>

#include "callctx.h"

#ifndef POOL_MALLOC
#define POOL_MALLOC(x) malloc(x)
#endif

#ifndef POOL_FREE
#define POOL_FREE(x) free(x)
#endif

#define pool_new(pool, chunk_sz, num_chunks) do {     \
  SET_CALLCTX();                    \
  _pool_new_impl((pool), (chunk_sz), (num_chunks));   \
} while (0)

#define pool_alloc(pool, out) do { \
  SET_CALLCTX(); \
  _pool_alloc_impl((pool), (out)); \
} while (0)

#define pool_free(pool, data) do { \
  SET_CALLCTX(); \
  _pool_free_impl((pool), (data)); \
} while (0)

typedef struct {
  void const* alloc;
  struct BlockHeader* first;
} Pool;

struct BlockHeader {
  uint8_t guard;
  struct BlockHeader* next;
};

static inline uintptr_t align_up(size_t al, uintptr_t a) {
  int align_m1 = al-1;
  uintptr_t b = a + align_m1;
  return b & ~align_m1;
}

void _pool_new_impl(Pool* p, size_t chunk_sz, uint64_t num_chunks); 
void _pool_alloc_impl(Pool* p, void** out);
void _pool_free_impl(Pool* p, void* data);
void pool_freeall(Pool* p);

#endif
