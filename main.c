#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  float sr;
} h_context_t;

typedef struct {
  float phase;
  float freq;
} h_oscillator_t;

float
h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = sinf(osc->phase);
  osc->phase += 2.0f * M_PI * osc->freq / ctx->sr;
  if (osc->phase > 2.0f * M_PI) osc->phase -= 2.0f * M_PI;
  return phase;
}

int
main()
{
  h_context_t ctx;
  h_oscillator_t osc;
  float *buf;
  size_t i;

  ctx.sr = 41000;

  osc.phase = 0.0f;
  osc.freq = ctx.sr;

  buf = malloc(sizeof(float) * ctx.sr * 10);
  for (i = 0; i < ctx.sr * 10; i++) {
    buf[i] = h_wave_sine(&osc, &ctx);
  }

  free(buf);
  return 0;
}
