#include <math.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    memcpy(buf + ctx->sample * 2, proc_compression.out, sizeof(float) * 2);
  }

  h_audio_free(&audio);

  return buf;
}

float *
my_noisy_synth(h_context_t *ctx, float duration)
{
  float *buf;
  h_noise_t noise = { .seed = 420 };
  h_oscillator_t osc_sine = h_osc_init(h_osc_sine, h_freq_from_midi(h_midi_from_note("A1")), 1.0f, 1.0f, 0.0f);
  h_oscillator_t lfo_square_freq = h_osc_init(h_osc_sine, 0.25f, 1.0f, 1.0f, 0.0f);
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
    h_osc(&lfo_square_freq, ctx);
    osc_sine.detune = lfo_square_freq.out[0] * M_PI;
    proc_bitcrush.bits = 16 - (floor((lfo_square_freq.out[0] + 1) * 8));

    h_audio(&audio, ctx);
    osc_sine.mod = (audio.out[0] + audio.out[1]) * 0.5f * 0.2f;

    h_osc(&osc_sine, ctx);
    h_proc_bitcrush(&proc_bitcrush, ctx, osc_sine.out);

    memcpy(buf + ctx->sample * 2, proc_bitcrush.out, sizeof(float) * 2);
  }

  h_audio_free(&audio);

  return buf;
}

float *
my_shaped_sine(h_context_t *ctx, float duration)
{
  float *buf, out[2], chebyshev_n;
  h_oscillator_t lfo_phase_mod = h_osc_init(h_osc_sine, 0.25f, 1.0f, 1.0f, 0.0f);
  h_oscillator_t lfo_chebyshev_n = h_osc_init(h_osc_sine, 0.10f, 1.0f, 1.0f, 0.0f);
  h_oscillator_t osc_sine = h_osc_init(h_osc_sine, h_freq_from_midi(h_midi_from_note("A2")), 1.0f, 1.0f, 0.4f);
  size_t i, j;
  h_shaper_t shaper_diode;
  h_shaper_t shaper_chebyshev;

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    /* lfo */
    h_osc(&lfo_phase_mod, ctx);
    h_osc(&lfo_chebyshev_n, ctx);

    for (i = 0; i < 2; i++) {
      out[i] = 0.0f;
    }

    h_osc(&osc_sine, ctx);
    osc_sine.mod = osc_sine.out[0] * (lfo_phase_mod.out[0] * 0.5f + 0.5f) * 4.0f;

    chebyshev_n = floorf((lfo_chebyshev_n.out[0] * 0.5f + 0.5f) * 6.0f) + 1.0f;

    for (i = 0; i <= chebyshev_n; i++) {
      h_shaper_chebyshev(&shaper_chebyshev, i, osc_sine.out);
      for (j = 0; j < 2; j++) {
        out[j] += shaper_chebyshev.out[j] * 1.0f / chebyshev_n;
      }
    }

    h_shaper_diode(&shaper_diode, out);

    memcpy(buf + ctx->sample * 2, shaper_diode.out, sizeof(float) * 2);
  }

  return buf;
}

float *
my_hypersaw(h_context_t *ctx, float duration)
{
  float *buf, frequency, initial_phase, stack_size = 7.0f;
  h_synth_t synth;
  size_t i;

  h_synth_init(&synth, 1);
  h_voice_init(synth.voices, stack_size, h_osc_sawtooth);

  frequency = h_freq_from_midi(h_midi_from_note("C4"));
  for (i = 0; i < stack_size; i++) {
    synth.voices->stack[i] = h_osc_init(h_osc_sawtooth, frequency - (i - 3) * 0.5f, 1.0f / stack_size, 1.0f, 0.0f);
  }

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    h_synth(&synth, ctx);
    memcpy(buf + ctx->sample * 2, synth.out, sizeof(float) * 2);
  }

  h_voice_free(synth.voices);
  h_synth_free(&synth);

  return buf;
}

float *
my_chords(h_context_t *ctx, float duration)
{
  float *buf, initial_phase;
  size_t voice_count = 4, i;
  h_synth_t chords_synth;
  h_synth_t bass_synth;
  h_proc_bitcrush_t chords_bitcrush = h_proc_bitcrush_init(18000.0f, 10);
  h_proc_bitcrush_t bass_bitcrush = h_proc_bitcrush_init(12000.0f, 8);
  h_shaper_t bass_shaper;
  h_note_t chords_notes[16], bass_notes[4];

  chords_notes[0] = (h_note_t) { .pitch = h_midi_from_note("C3"), .start = 0.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[1] = (h_note_t) { .pitch = h_midi_from_note("E3"), .start = 0.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[2] = (h_note_t) { .pitch = h_midi_from_note("G3"), .start = 0.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[3] = (h_note_t) { .pitch = h_midi_from_note("B3"), .start = 0.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[4] = (h_note_t) { .pitch = h_midi_from_note("A2"), .start = 2.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[5] = (h_note_t) { .pitch = h_midi_from_note("C3"), .start = 2.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[6] = (h_note_t) { .pitch = h_midi_from_note("E3"), .start = 2.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[7] = (h_note_t) { .pitch = h_midi_from_note("A3"), .start = 2.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[8] = (h_note_t) { .pitch = h_midi_from_note("D3"), .start = 4.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[9] = (h_note_t) { .pitch = h_midi_from_note("F3"), .start = 4.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[10] = (h_note_t) { .pitch = h_midi_from_note("A3"), .start = 4.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[11] = (h_note_t) { .pitch = h_midi_from_note("C4"), .start = 4.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[12] = (h_note_t) { .pitch = h_midi_from_note("B2"), .start = 6.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[13] = (h_note_t) { .pitch = h_midi_from_note("D3"), .start = 6.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[14] = (h_note_t) { .pitch = h_midi_from_note("A3"), .start = 6.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  chords_notes[15] = (h_note_t) { .pitch = h_midi_from_note("B3"), .start = 6.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };

  bass_notes[0] = (h_note_t) { .pitch = h_midi_from_note("C2"), .start = 0.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  bass_notes[1] = (h_note_t) { .pitch = h_midi_from_note("A1"), .start = 2.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  bass_notes[2] = (h_note_t) { .pitch = h_midi_from_note("D2"), .start = 4.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };
  bass_notes[3] = (h_note_t) { .pitch = h_midi_from_note("G2"), .start = 6.0f, .duration = 2.0f, .velocity = 1.0f, .detune = 0.0f, };

  h_synth_init(&chords_synth, voice_count);
  h_synth_init(&bass_synth, 1);

  for (i = 0; i < voice_count; i++) {
    h_voice_init(&chords_synth.voices[i], 1, h_osc_sine);
    chords_synth.voices[i].stack[0] = h_osc_init(h_osc_sine, 0.0f, h_amp_from_db(-16.0f), 0.0f, 0.0f);
  }

  h_voice_init(&bass_synth.voices[0], 1, h_osc_sine);
  bass_synth.voices[0].stack[0] = h_osc_init(h_osc_square, 0.0f, h_amp_from_db(-20.0f), 0.0f, 0.1f);

  buf = malloc(sizeof(float) * ctx->sr * duration * 2);
  if (0 == buf) {
    return 0;
  }

  for (ctx->sample = 0; ctx->sample < ctx->sr * duration; ctx->sample++) {
    h_note(&chords_synth, ctx, chords_notes, 16);
    h_note(&bass_synth, ctx, bass_notes, 4);

    h_proc_bitcrush(&chords_bitcrush, ctx, chords_synth.out);
    h_proc_bitcrush(&bass_bitcrush, ctx, bass_synth.out);
    h_shaper_foldback(&bass_shaper, 0.7f, bass_bitcrush.out);

    for (i = 0; i < 2; i++) {
      buf[ctx->sample * 2 + i] = chords_bitcrush.out[i] + bass_shaper.out[i];
    }
  }

  for (i = 0; i < voice_count; i++) {
    h_voice_free(&chords_synth.voices[i]);
  }
  h_voice_free(&bass_synth.voices[0]);

  h_synth_free(&chords_synth);
  h_synth_free(&bass_synth);

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

  duration = 8;
  buf = my_chords(&ctx, duration);
  if (0 == buf) {
    fprintf(stderr, "could not synthesize chords\n");
    return -1;
  }
  h_save_wav32("chords.wav", ctx.sr, ctx.sr * duration * 2, buf);
  free(buf);

  return 0;
}
