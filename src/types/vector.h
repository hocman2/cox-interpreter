#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdlib.h>

#define vector_new(v, cap) \
do { \
  (v).count = 0; \
  (v).capacity = (cap == 0) ? 1 : cap; \
  (v).xs = malloc(cap * sizeof(*(v).xs)); \
} while (0)

#define vector_push(v, x) \
do { \
  if ((v).count == (v).capacity) { \
    (v).capacity *= 2; \
    (v).xs = realloc((v).xs, sizeof(*(v).xs) * (v).capacity); \
  } \
  (v).xs[(v).count] = x;\
  (v).count += 1; \
} while (0)

#define vector_pop(v) \
do { \
  (v).count -= 1; \
} while (0)

#define vector_free(v) \
do { \
  free((v).xs); \
} while (0)

#endif
