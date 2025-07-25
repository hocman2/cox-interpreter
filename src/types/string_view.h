#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

#include <stdlib.h>

typedef struct {
  const char* str;
  size_t len;
} StringView;

#endif
