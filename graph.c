#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "harsh.h"

static inline void
process_math_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_math_t data = node->data.math;
  h_graph_node_t *left, *right;

  left = h_hm_get(g, data.left);
  right = h_hm_get(g, data.right);
  h_graph_process_node(g, left, ctx);
  h_graph_process_node(g, right, ctx);

  switch (data.op) {
  case H_NODE_MATH_ADD:
    node->out = left->out + right->out;
    break;
  case H_NODE_MATH_MUL:
    node->out = left->out * right->out;
    break;
  }
}

static inline uint32_t
xorshift32(uint32_t *state)
{
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return *state = x;
}

static inline void
process_noise_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_noise_t *data = &node->data.noise;
  h_graph_node_t *seed;

  seed = h_hm_get(g, data->seed);
  h_graph_process_node(g, seed, ctx);
  data->state ^= (unsigned int) seed->out;

  float u1 = ((float) xorshift32(&data->state) + 1.0f) / ((float) UINT32_MAX + 2.0f);
  float u2 = ((float) xorshift32(&data->state) + 1.0f) / ((float) UINT32_MAX + 2.0f);
  node->out = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

static inline void
process_osc_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_osc_t *data = &node->data.osc;
  h_graph_node_t *freq, *phase;

  freq = h_hm_get(g, data->freq);
  phase = h_hm_get(g, data->phase);
  h_graph_process_node(g, freq, ctx);
  h_graph_process_node(g, phase, ctx);

  switch (data->type) {
  case H_NODE_OSC_SINE:
    node->out = sinf(data->current + phase->out);
    data->current += 2.0f * M_PI * freq->out / ctx->sr;
    data->current = fmod(data->current, 2.0f * M_PI);
    if (data->current < 0.0f) data->current += 2.0f * M_PI;
    break;
  case H_NODE_OSC_SQUARE:
    node->out = (data->current + phase->out < 0.5 ? -1.0f : 1.0f);
    data->current += freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
    break;
  case H_NODE_OSC_SAWTOOTH:
    node->out = (1.0f - 2.0f + phase->out);
    data->current += freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
    break;
  }
}

static inline void
process_diode_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_graph_node_t *input;

  input = h_hm_get(g, node->data.diode);
  h_graph_process_node(g, input, ctx);

  node->out = log1pf(expf(input->out));
}

static inline void
process_bitcrush_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_bitcrush_t *data = &node->data.bitcrush;
  h_graph_node_t *input, *target_freq, *bits;
  float levels, norm, quantized;

  input = h_hm_get(g, data->input);
  target_freq = h_hm_get(g, data->target_freq);
  bits = h_hm_get(g, data->bits);
  h_graph_process_node(g, input, ctx);
  h_graph_process_node(g, target_freq, ctx);
  h_graph_process_node(g, bits, ctx);

  data->current_freq += target_freq->out;
  if (data->current_freq < ctx->sr) {
    return;
  }
  data->current_freq -= ctx->sr;

  levels = powf(2.0f, bits->out);
  if (levels <= 1.0f) {
    node->out = 0.0f;
    return;
  }

  norm = (input->out + 1.0f) * 0.5f;
  quantized = floorf(norm * levels) / (levels - 1.0f);
  node->out = quantized * 2.0f - 1.0f;
}

void
h_graph_process_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  if (node->last_frame == ctx->current_frame) {
    return;
  }

  switch (node->type) {
  case H_NODE_VALUE:
    break;
  case H_NODE_MATH:
    process_math_node(g, node, ctx);
    break;
  case H_NODE_NOISE:
    process_noise_node(g, node, ctx);
    break;
  case H_NODE_OSC:
    process_osc_node(g, node, ctx);
    break;
  case H_NODE_DIODE:
    process_diode_node(g, node, ctx);
    break;
  case H_NODE_BITCRUSH:
    process_bitcrush_node(g, node, ctx);
    break;
  }

  node->last_frame = ctx->current_frame;
}

inline static void
graph_preview(const char *prefix, h_hm_t *g, h_graph_node_t *node, size_t depth)
{
  char margin[32];

  margin[0] = '|';

  memset(margin + 1, '-', (depth + 1) * 2);
  margin[(depth + 1) * 2] = '\0';
  fprintf(stderr, "%s %s%s ", margin, prefix, node->name);

  switch (node->type) {
  case H_NODE_VALUE:
    fprintf(stderr, "(literal %f)\n", node->out);
    break;
  case H_NODE_MATH:
    fprintf(stderr, "(math)\n");
    graph_preview("left:", g, h_hm_get(g, node->data.math.left), depth + 1);
    graph_preview("right:", g, h_hm_get(g, node->data.math.right), depth + 1);
    break;
  case H_NODE_NOISE:
    fprintf(stderr, "(noise)\n");
    graph_preview("seed:", g, h_hm_get(g, node->data.noise.seed), depth + 1);
    break;
  case H_NODE_OSC:
    fprintf(stderr, "(osc)\n");
    graph_preview("freq:", g, h_hm_get(g, node->data.osc.freq), depth + 1);
    graph_preview("phase:", g, h_hm_get(g, node->data.osc.phase), depth + 1);
    break;
  case H_NODE_DIODE:
    fprintf(stderr, "(diode)\n");
    graph_preview("input:", g, h_hm_get(g, node->data.diode), depth + 1);
    break;
  case H_NODE_BITCRUSH:
    fprintf(stderr, "(bitcrush)\n");
    graph_preview("input:", g, h_hm_get(g, node->data.bitcrush.input), depth + 1);
    graph_preview("target_freq:", g, h_hm_get(g, node->data.bitcrush.target_freq), depth + 1);
    graph_preview("bits:", g, h_hm_get(g, node->data.bitcrush.bits), depth + 1);
    break;
  }
}

void
h_graph_preview(h_hm_t *g)
{
  graph_preview("", g, h_hm_get(g, "output"), 0);
}

void
h_graph_render_block(h_hm_t *g, h_graph_node_t *out, h_context *ctx, float *buf, size_t buf_size)
{
  size_t i;

  for (i = 0; i < buf_size; i++) {
    h_graph_process_node(g, out, ctx);
    buf[i] = out->out;
    ctx->current_frame++;
  }
}

void
h_graph_free(h_hm_t *g)
{
  size_t i;
  h_hm_entry_t *entry;

  for (i = 0; i < g->capacity; i++) {
    entry = g->buckets[i];
    while (entry) {
      free(entry->value);
      entry = entry->next;
    }
  }

  h_hm_free(g);
}
