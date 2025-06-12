#include <math.h>
#include <stdlib.h>

#include "harsh.h"

float
h_wave_noise()
{
  float u1 = ((float) rand() + 1.0f) / ((float) RAND_MAX + 2.0f);
  float u2 = ((float) rand() + 1.0f) / ((float) RAND_MAX + 2.0f);

  return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

float
h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx, float freq)
{
  float phase = sinf(osc->phase);
  osc->phase += 2.0f * M_PI * freq / ctx->sr;
  if (osc->phase > 2.0f * M_PI) osc->phase -= 2.0f * M_PI;
  return phase;
}

float
h_wave_square(h_oscillator_t *osc, const h_context_t *ctx, float freq)
{
  float phase = osc->phase < 0.5 ? 1.0f : -1.0f;
  osc->phase += freq / ctx->sr;
  if (osc->phase > 1.0f) osc->phase -= 1.0f;
  return phase;
}
