#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdlib.h>

// simple ugly dynamic array struct, not meant to be optimized

typedef struct {
  void* els;
  size_t num_els;
  size_t _cap;
  size_t _type_sz;
} Vector;

static Vector vector_new(size_t initial_cap, size_t type_sz) {
  if (initial_cap == 0) initial_cap = 1;

  Vector v = {
    malloc(type_sz * initial_cap),
    0,
    initial_cap,
    type_sz
  };

  return v;
}

static void vector_push(Vector* v, void* el) {
  if (v->num_els == v->_cap) {
    v->_cap *= 2;
    v->els = realloc(v->els, v->_type_sz * v->_cap);
  }

  void* insertion_loc = v->els + (v->num_els * v->_type_sz);
  memcpy(insertion_loc, el, v->_type_sz);
  v->num_els += 1;
}

static void vector_pop(Vector* v) {
  v->num_els -= 1;
}

#endif
