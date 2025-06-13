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
h_wave_sine(float *p, const h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = sinf(*p + osc->mod);
  *p += 2.0f * M_PI * osc->freq / ctx->sr;
  if (*p > 2.0f * M_PI) *p -= 2.0f * M_PI;
  return phase;
}

float
h_wave_square(float *p, const h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = (*p + osc->mod) < 0.5 ? 1.0f : -1.0f;
  *p += osc->freq / ctx->sr;
  if (*p > 1.0f) *p -= 1.0f;
  return phase;
}
