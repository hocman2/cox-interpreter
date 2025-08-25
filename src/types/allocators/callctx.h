#ifndef _CALLCTX_H
#define _CALLCTX_H

#ifdef _DEBUG
#include <string.h>

#define SET_CALLCTX() do { \
  strcpy(g_callctx.file, __FILE__); \
  g_callctx.line = __LINE__; \
} while (0)
#define CTX_FILE g_callctx.file
#define CTX_LINE g_callctx.line

struct CallerCtx {
  char file[1024];
  int line;
};

struct CallerCtx g_callctx;

#else

#define SET_CALLCTX() do{}while(0)
#define CTX_FILE
#define CTX_LINE

#endif

#endif
