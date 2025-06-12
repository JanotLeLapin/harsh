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
  h_oscillator_t sine, square;
  float *buf, notes[4], duration = 3, c2, bass_pitch, phase;

  ctx.sample = 0;
  ctx.sr = 41000;

  sine.phase = 0.0f;
  square.phase = M_PI * 10;

  notes[0] = h_freq_from_midi(h_midi_from_note("C4"));
  notes[1] = h_freq_from_midi(h_midi_from_note("E4"));
  notes[2] = h_freq_from_midi(h_midi_from_note("G4"));
  notes[3] = h_freq_from_midi(h_midi_from_note("B4"));

  c2 = h_freq_from_midi(h_midi_from_note("C2"));
  bass_pitch = 3 * c2;

  buf = malloc(sizeof(float) * ctx.sr * duration);
  for (ctx.sample = 0; ctx.sample < ctx.sr * duration; ctx.sample++) {
    phase = h_wave_sine(&sine, &ctx, notes[(int) floorf(ctx.sample / ctx.sr / duration * 4)]);
    buf[ctx.sample] = (phase < 0.0f ? 0.0f : phase) * 0.4 + h_wave_square(&square, &ctx, bass_pitch) * 0.3;
    bass_pitch -= ctx.sample * 12 / ctx.sr;
    if (bass_pitch < c2) bass_pitch = c2;
  }

  h_save_wav32("out.wav", ctx.sr, ctx.sr * duration, buf);

  free(buf);
  return 0;
}
