#include "pool.h"
#include "callctx.h"

#include <stdio.h>

#define GUARD_BYTE 0xFF

#ifdef _DEBUG
#define FATAL_ERR(condition, fmtmsg) \
if (condition) { \
  fprintf(stderr, "[Pool allocator] "fmtmsg" @ %s:%d\n", CTX_FILE, CTX_LINE); \
  exit(1); \
}
#else
#define FATAL_ERR(condition, fmtmsg) \
if (condition) { \
  fprintf(stderr, "[Pool allocator] "fmtmsg"\n"); \
  exit(1); \
}
#endif

void _pool_new_impl(Pool* p, size_t chunk_sz, uint64_t num_chunks) {
  FATAL_ERR(chunk_sz == 0, "Can't create a pool of 0 sized chunks !");
  FATAL_ERR(num_chunks == 0, "Can't create a pool of 0 chunks !");

  size_t total_chunk_sz = chunk_sz + sizeof(struct BlockHeader);
  size_t total_sz = total_chunk_sz * num_chunks;

  void* data = POOL_MALLOC(total_sz);
  FATAL_ERR(data == NULL, "Initial memory allocation failed !");

  for (size_t i = 0; i < num_chunks-1; ++i) {
    struct BlockHeader* node_start = data + (i * total_chunk_sz);
    struct BlockHeader* node_next = (node_start + total_chunk_sz);

    *node_start = (struct BlockHeader){GUARD_BYTE, node_next};
  }
  
  *p = (Pool){
    .alloc = data, // for freeing
    .first = (struct BlockHeader*)data,
  };
}

void _pool_alloc_impl(Pool* p, void** out) {
  struct BlockHeader* data_start = p->first;
  FATAL_ERR(data_start == NULL, "Pool is out of memory.");
  FATAL_ERR(data_start->guard != GUARD_BYTE, "A pool block header has been corrupted !");

  p->first = data_start->next;
  *out = (void*)data_start + sizeof(struct BlockHeader);
}

void _pool_free_impl(Pool* p, void* data) {
  FATAL_ERR(p->first && (p->first->guard != GUARD_BYTE), "A pool block header has been corrupted !");

  struct BlockHeader* data_node = data - sizeof(struct BlockHeader);
  FATAL_ERR(data_node->guard != GUARD_BYTE, "Data block header has been corrupted !");

  data_node->next = p->first;
  p->first = data_node;
}

void pool_freeall(Pool* p) {
  POOL_FREE((void*)p->alloc); // const ditching is fine here
}
