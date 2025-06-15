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
h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = sinf(osc->phase + osc->mod);
  osc->phase += 2.0f * M_PI * osc->freq / ctx->sr;
  if (osc->phase > 2.0f * M_PI) osc->phase -= 2.0f * M_PI;
  return phase;
}

float
h_wave_square(h_oscillator_t *osc, const h_context_t *ctx)
{
  float tmp, phase;
  tmp = fmodf(osc->phase + osc->mod, 1.0f);
  if (tmp < 0.0f) tmp += 1.0f;
  phase = tmp < 0.5 ? 1.0f : -1.0f;
  osc->phase += osc->freq / ctx->sr;
  if (osc->phase > 1.0f) osc->phase -= 1.0f;
  return phase;
}

float
h_wave_sawtooth(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = (osc->phase + osc->mod);
  osc->phase += osc->freq / ctx->sr * 2;
  if (osc->phase > 1.0f) osc->phase -= 2.0f;
  return phase;
}
