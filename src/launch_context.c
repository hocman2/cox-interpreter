#include "launch_context.h"
#include <string.h>
#include <stdint.h>

LaunchContext g_launch_ctx = {0};

LaunchContext* launch_ctx_new(char* opts[], int optsc) {
  for (size_t i = 0; i < (size_t)optsc; ++i) {
    const char* opt = opts[i];

    if (0) {}
    else if (strcmp(opt, "--report-scopes") == 0) {
      g_launch_ctx.print_scopes = true;
    }
  }

  return &g_launch_ctx;
}

LaunchContext* launch_ctx_get() {
  return &g_launch_ctx;
}
