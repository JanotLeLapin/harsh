#ifndef _H_HARSH
#define _H_HARSH

typedef struct {
  unsigned long sample;
  float sr;
} h_context_t;

typedef struct {
  float phase;
} h_oscillator_t;

/* oscillators */
float h_wave_noise();
float h_wave_sine(h_oscillator_t *osc, const h_context_t *ctx, float freq);
float h_wave_square(h_oscillator_t *osc, const h_context_t *ctx, float freq);

#endif
