#include <stdlib.h>

#include "harsh.h"

void
h_note(h_synth_t *synth, const h_context_t *ctx, h_note_t *notes, size_t note_count)
{
  size_t i, j, voice_index = 0;
  float time = ctx->sample / ctx->sr;
  h_voice_t *voice;

  for (i = 0; i < note_count; i++) {
    if (time < notes[i].start || time > notes[i].start + notes->duration) {
      continue;
    }
    voice = &synth->voices[(voice_index++) % synth->voice_count];
    for (j = 0; j < voice->stack_size; j++) {
      voice->stack[j].freq = h_freq_from_midi(notes[i].pitch);
    }
  }

  h_synth(synth, ctx);
}
