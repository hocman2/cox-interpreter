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
  return a;
}

static void* arena_alloc(Arena* a, size_t sz) {
  // Alloc wont fit !
  if (a->offset+sz > a->buf_sz) {
    fprintf(stderr, "Arena allocator: Out of memory !");
    exit(-1);
    //return NULL;
  }

  uintptr_t ret = (uintptr_t)a->buf + a->offset; 
  a->offset += sz;
  return (void*)ret;
}

static void arena_reset(Arena* a) {
  a->offset = 0;
}

static void arena_free(Arena* a) {
  free(a->buf);
}

#endif
