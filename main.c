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
  sfinfo.channels = 2;
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
  h_noise_t noise = { .seed = 420 };
  h_oscillator_t osc_square = H_OSC_INIT(h_freq_from_midi(h_midi_from_note("A1")), 1.0f, 0.0f);
  h_oscillator_t lfo_square_freq = H_OSC_INIT(0.25f, 1.0f, 0.0f);
  h_proc_bitcrush_t proc_bitcrush = H_BITCRUSH_INIT(2048.0f * M_PI, 32);
  float *buf, duration = 10;

  ctx.sample = 0;
  ctx.sr = 41000;

  buf = malloc(sizeof(float) * ctx.sr * duration * 2);
  for (ctx.sample = 0; ctx.sample < ctx.sr * duration; ctx.sample++) {
    lfo_square_freq.mod = h_wave_noise(&noise) * 0.08f;
    h_wave_sine(&lfo_square_freq, &ctx);
    osc_square.detune = lfo_square_freq.out[0] * M_PI;
    proc_bitcrush.bits = 32 - (floor((lfo_square_freq.out[0] + 1) * 16));

    h_wave_sine(&osc_square, &ctx);
    h_proc_bitcrush(&proc_bitcrush, &ctx, osc_square.out);

    buf[ctx.sample * 2] = proc_bitcrush.out[0];
    buf[ctx.sample * 2 + 1] = proc_bitcrush.out[1];
  }

  h_save_wav32("out.wav", ctx.sr, ctx.sr * duration * 2, buf);

  free(buf);
  return 0;
}
