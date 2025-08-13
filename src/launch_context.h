#ifndef _LAUNCH_CONTEXT_H
#define _LAUNCH_CONTEXT_H

typedef struct {
  bool print_scopes;
} LaunchContext;

extern LaunchContext g_launch_ctx;

LaunchContext* launch_ctx_new(char* opts[], int optsc);
LaunchContext* launch_ctx_get();

#endif
