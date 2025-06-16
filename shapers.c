#include <stdlib.h>

#include "harsh.h"

void
h_shaper_diode(h_shaper_t *shaper, float input[2])
{
  size_t i;
  for (i = 0; i < 2; i++) {
    shaper->out[i] = log1pf(expf(input[i]));
  }
}

static inline float
chebyshev(int n, float input)
{
  switch (n) {
  case 0:
    return 1;
  case 1:
    return input;
  default:
    return 2 * input * chebyshev(n - 1, input) - chebyshev(n - 2, input);
  }
}

void
h_shaper_chebyshev(h_shaper_t *shaper, int n, float *input)
{
  size_t i;
  for (i = 0; i < 2; i++) {
    shaper->out[i] = chebyshev(n, input[i]);
  }
}

void
h_shaper_foldback(h_shaper_t *shaper, float threshold, float *input)
{
  size_t i;
  float abs_in, diff;
  for (i = 0; i < 2; i++) {
    abs_in = fabs(input[i]);
    diff = abs_in - threshold;
    if (diff < 0) {
      shaper->out[i] = input[i];
      continue;
    }
    shaper->out[i] = threshold - fabs(fabs(fmod(input[i] - threshold, threshold * 4)) - threshold * 2);
  }
}
