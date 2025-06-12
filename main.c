#include <math.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>

#include "harsh.h"

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

  ctx.sample = 0;
  ctx.sr = 41000;

  osc.phase = 0.0f;
  osc.freq = 440;

  buf = malloc(sizeof(float) * ctx.sr * 10);
  for (ctx.sample = 0; ctx.sample < ctx.sr * 10; ctx.sample++) {
    buf[ctx.sample] = h_wave_sine(&osc, &ctx);
  }

  h_save_wav32("out.wav", ctx.sr, ctx.sr * 10, buf);

  free(buf);
  return 0;
}
