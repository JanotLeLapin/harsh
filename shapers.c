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
