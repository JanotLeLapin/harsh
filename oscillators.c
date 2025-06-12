#include <math.h>

#include "harsh.h"

float
h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = sinf(osc->phase);
  osc->phase += 2.0f * M_PI * osc->freq / ctx->sr;
  if (osc->phase > 2.0f * M_PI) osc->phase -= 2.0f * M_PI;
  return phase;
}

float
h_wave_square(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = osc->phase < 0.5 ? 1.0f : -1.0f;
  osc->phase += osc->freq / ctx->sr;
  if (osc->phase > 1.0f) osc->phase -= 1.0f;
  return phase;
}

