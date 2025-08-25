#include "string_view.h"
#include <string.h>

StringView sv_newn(const char* str, size_t n) {
  return (StringView){ str, n };
}

StringView sv_new(const char* str) {
  return sv_newn(str, strlen(str));
}
