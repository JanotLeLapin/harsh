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

void
h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx)
{
  size_t i;
  for (i = 0; i < 2; i++) {
    osc->out[i] = sinf(osc->phase[i] + osc->mod);
    osc->phase[i] += 2.0f * M_PI * (osc->freq + (i * 2.0f - 1.0f) * osc->detune) / ctx->sr;
    osc->phase[i] = fmod(osc->phase[i], 2.0f * M_PI);
    if (osc->phase[i] < 0.0f) osc->phase[i] += 2.0f * M_PI;
  }
}

void
h_wave_square(h_oscillator_t *osc, const h_context_t *ctx)
{
  size_t i;
  for (i = 0; i < 2; i++) {
    osc->out[i] = osc->phase[i] < 0.5 ? -1.0f : 1.0f;
    osc->phase[i] += (osc->freq + (i * 2.0f - 1.0f) * osc->detune) / ctx->sr;
    osc->phase[i] = fmodf(osc->phase[i], 1.0f);
    if (osc->phase[i] < 0.0f) osc->phase[i] += 1.0f;
  }
}

void
h_wave_sawtooth(h_oscillator_t *osc, const h_context_t *ctx)
{
  size_t i;
  for (i = 0; i < 2; i++) {
    osc->out[i] = 1.0f - 2.0f * osc->phase[i];
    osc->phase[i] += (osc->freq + (i * 2.0f - 1.0f) * osc->detune) / ctx->sr;
    osc->phase[i] = fmodf(osc->phase[i], 1.0f);
    if (osc->phase[i] < 0.0f) osc->phase[i] += 1.0f;
  }
}
