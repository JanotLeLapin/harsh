#ifndef _H_HARSH
#define _H_HARSH

#include <math.h>

typedef struct {
  unsigned long sample;
  float sr;
} h_context_t;

typedef struct {
  float freq;
  float mod;
} h_oscillator_t;

typedef struct {
  float latest_phase;
  float current_freq;
} h_proc_bitcrush_t;

/* util */
inline static float
h_freq_from_midi(char midi)
{
  return 440 * powf(2, ((float) midi - 69) / 12);
}

inline static char
h_midi_from_freq(float freq)
{
  return 69 + 12 * log2f(freq / 440);
}

inline static char
h_midi_from_note(const char *note)
{
  char sharp, octave, result;

  sharp = '#' == note[1];
  octave = note[sharp ? 2 : 1] - 48;

  result = sharp;

  switch (note[0]) {
    case 'C': result += 0; break;
    case 'D': result += 2; break;
    case 'E': result += 4; break;
    case 'F': result += 5; break;
    case 'G': result += 7; break;
    case 'A': result += 9; break;
    case 'B': result += 11; break;
    default: return -1;
  }

  return result + (octave + 1) * 12;
}

/* oscillators */
float h_wave_noise();
float h_wave_sine(float *p, const h_oscillator_t *osc, const h_context_t *ctx);
float h_wave_square(float *p, const h_oscillator_t *osc, const h_context_t *ctx);
float h_wave_sawtooth(float *p, const h_oscillator_t *osc, const h_context_t *ctx);

/* processors */
float h_proc_bitcrush(h_proc_bitcrush_t *proc, const h_context_t *ctx, float input, float target_freq, unsigned char bits);

#endif
