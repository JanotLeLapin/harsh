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
  h_oscillator_t osc_square = { .freq = 55.0f, .mod = 1.0f };
  h_oscillator_t lfo_square_freq = { .freq = sqrtf(2) * 0.1, .mod = 1.0f };
  h_proc_bitcrush_t proc_bitcrush = { .current_freq = 0.0f, .latest_phase = 0.0f };
  float *buf, duration = 8, osc_square_phase = 0.0f, lfo_square_freq_phase = M_PI;

  ctx.sample = 0;
  ctx.sr = 41000;

  buf = malloc(sizeof(float) * ctx.sr * duration);
  for (ctx.sample = 0; ctx.sample < ctx.sr * duration; ctx.sample++) {
    osc_square.mod = h_wave_noise() * (h_wave_sine(&lfo_square_freq_phase, &lfo_square_freq, &ctx) + 1) * 0.1;
    buf[ctx.sample] = h_proc_bitcrush(&proc_bitcrush, &ctx, h_wave_square(&osc_square_phase, &osc_square, &ctx), 18000.0f, 4);
  }

  h_save_wav32("out.wav", ctx.sr, ctx.sr * duration, buf);

  free(buf);
  return 0;
}
