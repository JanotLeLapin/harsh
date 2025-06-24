#include <math.h>
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
    node->out = (data->current < 0.5 ? -1.0f : 1.0f);
    data->current += freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
    break;
  case H_NODE_OSC_SAWTOOTH:
    node->out = (1.0f - 2.0f * phase->out);
    data->current += freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
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

inline static void
graph_preview(const char *prefix, h_hm_t *g, h_graph_node_t *node, size_t depth)
{
  char margin[32];
  size_t i;

  memset(margin, ' ', depth);
  margin[depth] = '\0';
  fprintf(stderr, "%s%s%s ", margin, prefix, node->name);

  switch (node->type) {
  case H_NODE_VALUE:
    fprintf(stderr, "(literal %f)\n", node->out);
    break;
  case H_NODE_MATH:
    fprintf(stderr, "(math)\n");
    graph_preview("left:", g, h_hm_get(g, node->data.math.left), depth + 1);
    graph_preview("right:", g, h_hm_get(g, node->data.math.right), depth + 1);
    break;
  case H_NODE_OSC:
    fprintf(stderr, "(osc)\n");
    graph_preview("freq:", g, h_hm_get(g, node->data.osc.freq), depth + 1);
    graph_preview("phase:", g, h_hm_get(g, node->data.osc.phase), depth + 1);
    break;
  }
}

void
h_graph_preview(h_hm_t *g)
{
  graph_preview("", g, h_hm_get(g, "output"), 0);
}

void
h_graph_free(h_hm_t *g)
{
  size_t i;
  h_hm_entry_t *entry;

  for (i = 0; i < g->size; i++) {
    entry = g->buckets[i];
    while (entry) {
      free(entry->value);
      entry = entry->next;
    }
  }

  h_hm_free(g);
}
