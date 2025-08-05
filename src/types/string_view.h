#ifndef _STRING_VIEW_H
#define _STRING_VIEW_H

#include <stdlib.h>

#define SV_Fmt "%.*s"
#define SV_Fmt_arg(x) (int)((x).len), ((x).str)

typedef struct {
  const char* str;
  size_t len;
} StringView;

#endif
