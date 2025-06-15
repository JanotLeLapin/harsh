#include <stdint.h>
#include <stdlib.h>

#include "harsh.h"

void
h_proc_bitcrush(h_proc_bitcrush_t *proc, const h_context_t *ctx, float input[2])
{
  size_t i;
  uint32_t encoded;

  proc->current_freq += proc->target_freq;
  if (proc->current_freq < ctx->sr) {
    return;
  }

  proc->current_freq -= ctx->sr;
  for (i = 0; i < 2; i++) {
    encoded = ((uint32_t) (((input[i] + 1) * 0.5) * UINT32_MAX)) >> (32 - proc->bits);
    proc->out[i] = ((float) (encoded << (32 - proc->bits)) / UINT32_MAX) * 2 - 1;
  }
}
