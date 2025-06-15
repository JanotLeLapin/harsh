#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sndfile.h>

#include "harsh.h"

int
h_audio_load(h_audio_t *audio, const char *filename)
{
  SF_INFO sfinfo = {0};
  SNDFILE *f;
  sf_count_t read_count;

  f = sf_open(filename, SFM_READ, &sfinfo);
  if (0 == f) {
    fprintf(stderr, "sf_open: %s\n", sf_strerror(NULL));
    return -1;
  }

  audio->sample_rate = (float) sfinfo.samplerate;
  audio->sample_count = (size_t) sfinfo.frames * sfinfo.channels;
  audio->samples = malloc(sizeof(float) * audio->sample_count);
  if (0 == audio->samples) {
    perror("malloc");
    sf_close(f);
    return -1;
  }

  read_count = sf_readf_float(f, audio->samples, sfinfo.frames);
  sf_close(f);
  if (read_count != sfinfo.frames) {
    fprintf(stderr, "sf_readf_float: incomplete read\n");
    return -1;
  }

  audio->out[0] = 0.0f;
  audio->out[1] = 0.0f;
  audio->current_sample = audio->start * audio->sample_rate;
  audio->current_freq = 0.0f;
  return 0;
}

void
h_audio_free(h_audio_t *audio)
{
  free(audio->samples);
  memset(audio, 0, sizeof(h_audio_t));
}

void
h_audio(h_audio_t *audio, const h_context_t *ctx)
{
  audio->current_freq += audio->sample_rate;
  if (audio->current_freq < ctx->sr) {
    return;
  }

  audio->current_freq -= ctx->sr;
  audio->out[0] = audio->samples[audio->current_sample * 2];
  audio->out[1] = audio->samples[audio->current_sample * 2 + 1];
  audio->current_sample++;

  if (audio->loop && audio->current_sample > (audio->start + audio->length) * audio->sample_rate) {
    audio->current_sample = audio->start * audio->sample_rate;
  }
}
