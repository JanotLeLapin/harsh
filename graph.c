#include <math.h>

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

static inline void
process_osc_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_osc_t data = node->data.osc;
  h_graph_node_t *freq, *phase;

  freq = h_hm_get(g, data.freq);
  phase = h_hm_get(g, data.phase);
  h_graph_process_node(g, freq, ctx);
  h_graph_process_node(g, phase, ctx);

  switch (data.type) {
  case H_NODE_OSC_SINE:
    node->out = sinf(data.current + phase->out);
    data.current += 2.0f * M_PI * freq->out / ctx->sr;
    data.current = fmod(phase->out, 2.0f * M_PI);
    if (data.current < 0.0f) data.current += 2.0f * M_PI;
    break;
  case H_NODE_OSC_SQUARE:
    node->out = (data.current < 0.5 ? -1.0f : 1.0f);
    data.current += freq->out / ctx->sr;
    data.current = fmodf(phase->out, 1.0f);
    if (data.current < 0.0f) data.current += 1.0f;
    break;
  case H_NODE_OSC_SAWTOOTH:
    node->out = (1.0f - 2.0f * phase->out);
    data.current += freq->out / ctx->sr;
    data.current = fmodf(data.current, 1.0f);
    if (data.current < 0.0f) data.current += 1.0f;
    break;
  }
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
  case H_NODE_OSC:
    process_osc_node(g, node, ctx);
    break;
  }

  node->last_frame = ctx->current_frame;
}
