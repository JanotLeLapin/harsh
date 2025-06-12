#include <math.h>
#include <sndfile.h>
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

float
h_wave_square(h_oscillator_t *osc, const h_context_t *ctx)
{
  float phase = osc->phase < 0.5 ? 1.0f : -1.0f;
  osc->phase += osc->freq / ctx->sr;
  if (osc->phase > 1.0f) osc->phase -= 1.0f;
  return phase;
}

int
h_save_wav32(const char *filename, uint32_t sample_rate, size_t sample_count, const float *samples)
{
  SF_INFO sfinfo;
  SNDFILE *f;

  sfinfo.frames = sample_count;
  sfinfo.samplerate = sample_rate;
  sfinfo.channels = 1;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

  f = sf_open(filename, SFM_WRITE, &sfinfo);
  if (0 == f) {
    perror("sf_open");
    return -1;
  }

  sf_write_float(f, samples, sample_count);
  sf_close(f);

  return 0;
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
  osc.freq = 440;

  buf = malloc(sizeof(float) * ctx.sr * 10);
  for (i = 0; i < ctx.sr * 10; i++) {
    buf[i] = h_wave_sine(&osc, &ctx);
  }

  h_save_wav32("out.wav", ctx.sr, ctx.sr * 10, buf);

  free(buf);
  return 0;
}
