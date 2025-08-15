#ifndef _VECTOR_H
#define _VECTOR_H

#include <stdlib.h>

#define vector_new(v, cap) \
do { \
  (v).count = 0; \
  (v).capacity = (cap == 0) ? 1 : cap; \
  (v).xs = malloc((v).capacity * sizeof(*(v).xs)); \
} while (0)

#define vector_empty(v) \
do { \
  (v).count = 0; \
} while (0)

#define vector_push(v, x) \
do { \
  if ((v).count == (v).capacity) { \
    (v).capacity *= 2; \
    (v).xs = realloc((v).xs, sizeof(*(v).xs) * (v).capacity); \
  } \
  (v).xs[(v).count] = (x);\
  (v).count += 1; \
} while (0)

#define vector_pop(v) \
do { \
  (v).count -= 1; \
} while (0)

#define vector_free(v) \
do { \
  free((v).xs); \
  (v).count = 0; \
  (v).capacity = 0; \
} while (0)

#define vector_copy(des, src) \
do { \
  vector_free(des); \
  (des).capacity = (src).capacity; \
  (des).count = (src).count; \
  (des).xs = malloc((src).count * sizeof(*(src).xs)); \
  for (size_t i = 0; i < (src).count; ++i) { \
    (des).xs[i] = (src).xs[i]; \
  } \
} while (0)

#define vector_from(des, src) \
do { \
  vector_new(des, (src).count); \
  (des).count = (src).count; \
  for (size_t i = 0; i < (src).count; ++i) { \
    (des).xs[i] = (src).xs[i]; \
  } \
} while (0)

#endif
