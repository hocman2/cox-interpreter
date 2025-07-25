#ifndef _ARENA_H
#define _ARENA_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
  size_t align;
  void* buf;
  size_t buf_sz;
  uintptr_t offset;
  size_t last_alloc_sz;
} Arena;

static uintptr_t align_up(size_t al, uintptr_t a) {
  int align_m1 = al-1;
  uintptr_t b = a + align_m1;
  return b & ~align_m1;
}

static Arena arena_init(size_t buf_sz, size_t align) {
  Arena a;
  a.buf = malloc(buf_sz);
  a.buf_sz = buf_sz;
  a.offset = 0;
  a.align = align;
  a.last_alloc_sz = 0;
  return a;
}

static void* arena_alloc(Arena* a, size_t sz) {
  uintptr_t curr_end = (uintptr_t)a->buf + a->offset;
  uintptr_t aligned = align_up(a->align, curr_end);

  size_t total_alloc_sz = (size_t)(aligned - curr_end + sz);
  
  // Alloc wont fit !
  if (a->offset+total_alloc_sz > a->buf_sz) {
    fprintf(stderr, "Arena allocator: Out of memory !\n");
    return NULL;
  }

  if (a->offset + total_alloc_sz < a->offset) {
    fprintf(stderr, "Arena allocator: size overflow !\n");
    return NULL;
  }

  a->offset += total_alloc_sz;
  a->last_alloc_sz = total_alloc_sz;

  return (void*)aligned;
}

/// Reverses the last allocation, making the data writable again
static void arena_pop(Arena* a, size_t sz) {
  a->offset -= a->last_alloc_sz;
}

static void arena_reset(Arena* a) {
  a->offset = 0;
}

static void arena_free(Arena* a) {
  free(a->buf);
}

#endif
