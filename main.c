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
  h_proc_compression_t proc_compression = h_proc_compression_init(h_amp_from_db(-60), INFINITY);
  h_audio_t audio = h_audio_init(0.0f, 20.0f, 1);

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
  h_oscillator_t osc_square = h_osc_init(h_freq_from_midi(h_midi_from_note("A1")), 1.0f, 0.0f);
  h_oscillator_t lfo_square_freq = h_osc_init(0.25f, 1.0f, 0.0f);
  h_proc_bitcrush_t proc_bitcrush = h_proc_bitcrush_init(2048.0f * M_PI, 32);
  h_audio_t audio = h_audio_init(10.5f, 1.8f, 1);

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
    h_osc_sine(&lfo_square_freq, ctx);
    osc_square.detune = lfo_square_freq.out[0] * M_PI;
    proc_bitcrush.bits = 16 - (floor((lfo_square_freq.out[0] + 1) * 8));

    h_audio(&audio, ctx);
    osc_square.mod = (audio.out[0] + audio.out[1]) * 0.5f * 0.2f;

    h_osc_sine(&osc_square, ctx);
    h_proc_bitcrush(&proc_bitcrush, ctx, osc_square.out);

    buf[ctx->sample * 2] = proc_bitcrush.out[0];
    buf[ctx->sample * 2 + 1] = proc_bitcrush.out[1];
  }

  h_audio_free(&audio);

  return buf;
}

float *
my_shaped_sine(h_context_t *ctx, float duration)
{
  float *buf, out[2], chebyshev_n;
  h_oscillator_t lfo_phase_mod = h_osc_init(0.25f, 1.0f, 0.0f);
  h_oscillator_t lfo_chebyshev_n = h_osc_init(0.10f, 1.0f, 0.0f);
  h_oscillator_t osc_sine = h_osc_init(h_freq_from_midi(h_midi_from_note("A2")), 1.0f, 0.4f);
  size_t i, j;
  h_shaper_t shaper_diode;
  h_shaper_t shaper_chebyshev;

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    /* lfo */
    h_osc_sine(&lfo_phase_mod, ctx);
    h_osc_sine(&lfo_chebyshev_n, ctx);

    for (i = 0; i < 2; i++) {
      out[i] = 0.0f;
    }

    h_osc_sine(&osc_sine, ctx);
    osc_sine.mod = osc_sine.out[0] * (lfo_phase_mod.out[0] * 0.5f + 0.5f) * 4.0f;

    chebyshev_n = floorf((lfo_chebyshev_n.out[0] * 0.5f + 0.5f) * 6.0f) + 1.0f;

    for (i = 0; i <= chebyshev_n; i++) {
      h_shaper_chebyshev(&shaper_chebyshev, i, osc_sine.out);
      for (j = 0; j < 2; j++) {
        out[j] += shaper_chebyshev.out[j] * 1.0f / chebyshev_n;
      }
    }

    h_shaper_diode(&shaper_diode, out);

    buf[ctx->sample * 2] = shaper_diode.out[0];
    buf[ctx->sample * 2 + 1] = shaper_diode.out[1];
  }

  return buf;
}

float *
my_hypersaw(h_context_t *ctx, float duration)
{
  float *buf, out[2], frequency, initial_phase, voice_count = 8.0f;
  h_noise_t noise = { .seed = 420 };
  h_oscillator_t lfo_detune;
  h_oscillator_t osc_sawtooth[8];
  size_t i;

  frequency = h_freq_from_midi(h_midi_from_note("C4"));
  lfo_detune = h_osc_init(0.1f, 1.0f, 0.0f);
  for (i = 0; i < voice_count; i++) {
    osc_sawtooth[i] = h_osc_init(0.0f, 1.0f, 0.0f);
    initial_phase = h_wave_noise(&noise);
    osc_sawtooth[i].phase[0] = initial_phase;
    osc_sawtooth[i].phase[1] = initial_phase;
  }

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    h_osc_sine(&lfo_detune, ctx);
    out[0] = 0.0f;
    out[1] = 0.0f;
    for (i = 0; i < voice_count; i++) {
      osc_sawtooth[i].freq = frequency - (i - (voice_count / 2.0f - 1.0f)) * lfo_detune.out[0] * 0.75f + (1.0f / 3.0f);
      h_osc_sawtooth(&osc_sawtooth[i], ctx);
      out[0] += osc_sawtooth[i].out[0] / voice_count;
      out[1] -= osc_sawtooth[i].out[0] / voice_count;
    }

    buf[ctx->sample * 2] = out[0];
    buf[ctx->sample * 2 + 1] = out[1];
  }

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

  buf = my_shaped_sine(&ctx, duration);
  if (0 == buf) {
    fprintf(stderr, "could not synthesize shaped sine\n");
    return -1;
  }
  h_save_wav32("shaped_sine.wav", ctx.sr, ctx.sr * duration * 2, buf);
  free(buf);

  buf = my_hypersaw(&ctx, duration);
  if (0 == buf) {
    fprintf(stderr, "could not synthesize hypersaw\n");
    return -1;
  }
  h_save_wav32("hypersaw.wav", ctx.sr, ctx.sr * duration * 2, buf);
  free(buf);

  return 0;
}
