#ifndef _H_HARSH
#define _H_HARSH

#include <math.h>

#define H_OSC_INIT(in_freq, in_mod, in_detune) { .out[0] = 0.0f, .out[1] = 0.0f, .phase[0] = 0.0f, .phase[1] = 0.0f, .freq = in_freq, .mod = in_mod, .detune = in_detune }

#define H_BITCRUSH_INIT(in_target_freq, in_bits) { .out[0] = 0.0f, .out[1] = 0.0f, .current_freq = 0.0f, .target_freq = in_target_freq, .bits = in_bits };

typedef struct {
  unsigned long sample;
  float sr;
} h_context_t;

/* oscillators */
typedef struct {
  float out[2];
  float phase[2];

  float freq;
  float mod;
  float detune;
} h_oscillator_t;

/* other signals */
typedef struct {
  unsigned int seed;
} h_noise_t;

/* processors */
typedef struct {
  float out[2];
  float current_freq;

  float target_freq;
  unsigned char bits;
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
void h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx);
void h_wave_square(h_oscillator_t *osc, const h_context_t *ctx);
void h_wave_sawtooth(h_oscillator_t *osc, const h_context_t *ctx);

/* other signals */
float h_wave_noise(h_noise_t *noise);

/* processors */
void h_proc_bitcrush(h_proc_bitcrush_t *proc, const h_context_t *ctx, float input[2]);

#endif
