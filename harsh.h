#ifndef _H_HARSH
#define _H_HARSH

typedef struct {
  unsigned long sample;
  float sr;
} h_context_t;

/* oscillators */
typedef struct {
  float phase;
  float freq;
} h_oscillator_t;

float h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx);
float h_wave_square(h_oscillator_t *osc, const h_context_t *ctx);

#endif
