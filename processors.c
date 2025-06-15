#include <stdint.h>

#include "harsh.h"

float
h_proc_bitcrush(h_proc_bitcrush_t *proc, const h_context_t *ctx, float input)
{
  uint32_t encoded;

  proc->current_freq += proc->target_freq;
  if (proc->current_freq >= ctx->sr) {
    proc->current_freq -= ctx->sr;
    encoded = ((uint32_t) (((input + 1) * 0.5) * UINT32_MAX)) >> (32 - proc->bits);
    proc->latest_phase = ((float) (encoded << (32 - proc->bits)) / UINT32_MAX) * 2 - 1;
  }

  return proc->latest_phase;
}
