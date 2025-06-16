#include <stdio.h>
#include <stdlib.h>

#include "harsh.h"

int
h_voice_init(h_voice_t *voice, size_t stack_size, h_osc_func_t osc)
{
  voice->out[0] = 0.0f;
  voice->out[1] = 0.0f;
  voice->stack = malloc(sizeof(h_oscillator_t) * stack_size);
  if (0 == voice->stack) {
    perror("malloc");
    return -1;
  }
  voice->stack_size = stack_size;
  voice->is_active = 0;
  voice->age = 0.0f;

  voice->osc = osc;
  voice->velocity = 1.0f;
  return 0;
}

void
h_voice(h_voice_t *voice, const h_context_t *ctx)
{
  size_t i, j;

  voice->out[0] = 0.0f;
  voice->out[1] = 0.0f;
  for (i = 0; i < voice->stack_size; i++) {
    voice->osc(&voice->stack[i], ctx);
    for (j = 0; j < 2; j++) {
      voice->out[j] += voice->stack[i].out[j];
    }
  }
}

void
h_voice_free(h_voice_t *voice)
{
  free(voice->stack);
}

int
h_synth_init(h_synth_t *synth, size_t voice_count)
{
  synth->out[0] = 0.0f;
  synth->out[1] = 0.0f;
  synth->voices = malloc(sizeof(h_voice_t) * voice_count);
  if (0 == synth->voices) {
    perror("malloc");
    return -1;
  }
  synth->voice_count = voice_count;
  return 0;
}

void
h_synth(h_synth_t *synth, const h_context_t *ctx)
{
  size_t i, j;

  synth->out[0] = 0.0f;
  synth->out[1] = 0.0f;
  for (i = 0; i < synth->voice_count; i++) {
    h_voice(&synth->voices[i], ctx);
    for (j = 0; j < 2; j++) {
      synth->out[j] += synth->voices[i].out[j];
    }
  }
}

void
h_synth_free(h_synth_t *synth)
{
  free(synth->voices);
}
