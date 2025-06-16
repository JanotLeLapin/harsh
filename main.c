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

float *
my_compressed_sample(h_context_t *ctx, float duration)
{
  float *buf;
  h_proc_compression_t proc_compression = H_COMPRESSION_INIT(h_amp_from_db(-60), INFINITY);
  h_audio_t audio = H_AUDIO_INIT(0.0f, 20.0f, 1);

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  if (-1 == h_audio_load(&audio, "audio.flac")) {
    free(buf);
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    h_audio(&audio, ctx);

    h_proc_compression(&proc_compression, ctx, audio.out);

    buf[ctx->sample * 2] = proc_compression.out[0] * 100.0f;
    buf[ctx->sample * 2 + 1] = proc_compression.out[1] * 100.0f;
  }

  h_audio_free(&audio);

  return buf;
}

float *
my_noisy_synth(h_context_t *ctx, float duration)
{
  float *buf;
  h_noise_t noise = { .seed = 420 };
  h_oscillator_t osc_square = H_OSC_INIT(h_freq_from_midi(h_midi_from_note("A1")), 1.0f, 0.0f);
  h_oscillator_t lfo_square_freq = H_OSC_INIT(0.25f, 1.0f, 0.0f);
  h_proc_bitcrush_t proc_bitcrush = H_BITCRUSH_INIT(2048.0f * M_PI, 32);
  h_audio_t audio = H_AUDIO_INIT(10.5f, 1.8f, 1);

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  if (-1 == h_audio_load(&audio, "audio.flac")) {
    free(buf);
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    lfo_square_freq.mod = h_wave_noise(&noise) * 0.08f;
    h_wave_sine(&lfo_square_freq, ctx);
    osc_square.detune = lfo_square_freq.out[0] * M_PI;
    proc_bitcrush.bits = 16 - (floor((lfo_square_freq.out[0] + 1) * 8));

    h_audio(&audio, ctx);
    osc_square.mod = (audio.out[0] + audio.out[1]) * 0.5f * 0.2f;

    h_wave_sine(&osc_square, ctx);
    h_proc_bitcrush(&proc_bitcrush, ctx, osc_square.out);

    buf[ctx->sample * 2] = proc_bitcrush.out[0];
    buf[ctx->sample * 2 + 1] = proc_bitcrush.out[1];
  }

  h_audio_free(&audio);

  return buf;
}

int
main()
{
  h_context_t ctx = { .sample = 0, .sr = 96000 };
  float *buf, duration = 10;

  buf = my_noisy_synth(&ctx, duration);
  if (0 == buf) {
    fprintf(stderr, "could not synthesize noisy synth\n");
    return -1;
  }
  h_save_wav32("noisy_synth.wav", ctx.sr, ctx.sr * duration * 2, buf);
  free(buf);

  buf = my_compressed_sample(&ctx, duration);
  if (0 == buf) {
    fprintf(stderr, "could not synthesize compressed sample\n");
    return -1;
  }
  h_save_wav32("compressed_sample.wav", ctx.sr, ctx.sr * duration * 2, buf);
  free(buf);

  return 0;
}
