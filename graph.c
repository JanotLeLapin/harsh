#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harsh.h"

static inline void
process_math_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_math_t data = node->data.math;
  size_t i = 0;
  h_graph_node_t *elem;

  switch (data.op) {
  case H_NODE_MATH_ADD:
    node->out = 0.0f;
    break;
  case H_NODE_MATH_MUL:
    node->out = 1.0f;
    break;
  case H_NODE_MATH_DIV:
  case H_NODE_MATH_POW:
    elem = *(h_graph_node_t **) h_vec_get(&data.values, i++);
    h_graph_process_node(g, elem, ctx);
    node->out = elem->out;
    break;
  case H_NODE_MATH_SUB:
    if (data.values.size > 1) {
      elem = *(h_graph_node_t **) h_vec_get(&data.values, i++);
      h_graph_process_node(g, elem, ctx);
      node->out = elem->out;
    } else {
      node->out = 0.0f;
    }
    break;
  default:
    break;
  }

  for (; i < data.values.size; i++) {
    elem = *(h_graph_node_t **) h_vec_get(&data.values, i);
    h_graph_process_node(g, elem, ctx);
    switch (data.op) {
    case H_NODE_MATH_ADD:
      node->out += elem->out;
      break;
    case H_NODE_MATH_SUB:
      node->out -= elem->out;
      break;
    case H_NODE_MATH_MUL:
      node->out *= elem->out;
      break;
    case H_NODE_MATH_DIV:
      node->out = 0.0f == elem->out ? 0.0f : node->out / elem->out;
      break;
    case H_NODE_MATH_POW:
      node->out = powf(node->out, elem->out);
      break;
    case H_NODE_MATH_LOGN:
      node->out = elem->out < 0.0f ? 0.0f : logf(elem->out);
      break;
    case H_NODE_MATH_LOG2:
      node->out = elem->out < 0.0f ? 0.0f : log2f(elem->out);
      break;
    case H_NODE_MATH_LOG10:
      node->out = elem->out < 0.0f ? 0.0f : log10f(elem->out);
      break;
    case H_NODE_MATH_EXP:
      node->out = expf(elem->out);
      break;
    }
  }
}

static inline void
process_cmp_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_cmp_t data = node->data.cmp;
  char res;

  h_graph_process_node(g, data.left, ctx);
  h_graph_process_node(g, data.right, ctx);

  switch (data.op) {
  case H_NODE_CMP_LT:
    res = data.left->out < data.right->out;
    break;
  case H_NODE_CMP_LEQT:
    res = data.left->out <= data.right->out;
    break;
  case H_NODE_CMP_GT:
    res = data.left->out > data.right->out;
    break;
  case H_NODE_CMP_GEQT:
    res = data.left->out >= data.right->out;
    break;
  case H_NODE_CMP_EQ:
    res = data.left->out == data.right->out;
    break;
  case H_NODE_CMP_NEQ:
    res = data.left->out != data.right->out;
    break;
  }

  node->out = res ? 1.0f : 0.0f;
}

static inline void
process_conversion_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_conversion_t data = node->data.conversion;

  h_graph_process_node(g, data.input, ctx);

  switch (data.op) {
  case H_NODE_CONVERSION_MTOF:
    node->out = 440.0f * powf(2.0f, (data.input->out - 69.0f) / 12.0f);
    break;
  case H_NODE_CONVERSION_FTOM:
    node->out = 69.0f + 12.0f * log2f(data.input->out / 440.0f);
    break;
  case H_NODE_CONVERSION_DTOA:
    node->out = 20.0f * log10f(data.input->out);
    break;
  case H_NODE_CONVERSION_ATOD:
    node->out = powf(10.0f, node->out / 20.0f);
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

  h_graph_process_node(g, data->seed, ctx);
  data->state ^= (unsigned int) data->seed->out;

  float u1 = ((float) xorshift32(&data->state) + 1.0f) / ((float) UINT32_MAX + 2.0f);
  float u2 = ((float) xorshift32(&data->state) + 1.0f) / ((float) UINT32_MAX + 2.0f);
  node->out = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
}

static inline void
process_osc_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_osc_t *data = &node->data.osc;

  h_graph_process_node(g, data->freq, ctx);
  h_graph_process_node(g, data->phase, ctx);

  switch (data->type) {
  case H_NODE_OSC_SINE:
    node->out = sinf(data->current + data->phase->out);
    data->current += 2.0f * M_PI * data->freq->out / ctx->sr;
    data->current = fmod(data->current, 2.0f * M_PI);
    if (data->current < 0.0f) data->current += 2.0f * M_PI;
    break;
  case H_NODE_OSC_SQUARE:
    node->out = (data->current + data->phase->out < 0.5 ? -1.0f : 1.0f);
    data->current += data->freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
    break;
  case H_NODE_OSC_SAWTOOTH:
    node->out = (1.0f - 2.0f + data->phase->out);
    data->current += data->freq->out / ctx->sr;
    data->current = fmodf(data->current, 1.0f);
    if (data->current < 0.0f) data->current += 1.0f;
    break;
  }
}

static inline void
process_diode_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_graph_process_node(g, node->data.diode, ctx);
  node->out = log1pf(expf(node->data.diode->out));
}

static inline void
process_hardclip_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_clip_t data = node->data.clip;

  h_graph_process_node(g, data.threshold, ctx);
  h_graph_process_node(g, data.input, ctx);

  switch (data.type) {
  case H_NODE_CLIP_HARDCLIP:
    node->out = fminf(fmaxf(data.input->out, -data.threshold->out), data.threshold->out);
    break;
  case H_NODE_CLIP_FOLDBACK:
    /* https://www.musicdsp.org/en/latest/Effects/203-fold-back-distortion.html */
    if (data.input->out > data.threshold->out || data.input->out < -data.threshold->out) {
      node->out = fabsf(fabsf(fmodf(data.input->out, data.threshold->out * 4)) - data.threshold->out * 2) - data.threshold->out;
    } else {
      node->out = data.input->out;
    }
    break;
  }
}

static inline void
process_filter_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_filter_t *data;
  float a, stage_in;
  int stages, i;

  data = &node->data.filter;

  h_graph_process_node(g, data->input, ctx);
  h_graph_process_node(g, data->stages, ctx);
  h_graph_process_node(g, data->cutoff, ctx);

  a = expf(-2.0f * M_PI * data->cutoff->out / ctx->sr);
  stages = (int) roundf(fmax(1.0, fmin(4.0, data->stages->out)));
  switch (data->type) {
  case H_NODE_FILTER_LOWPASS:
    stage_in = data->input->out;
    for (i = 0; i < stages; i++) {
      data->out[i] = (1 - a) * stage_in + a * data->out[i];
      stage_in = data->out[i];
    }
    break;
  case H_NODE_FILTER_HIGHPASS:
    stage_in = data->input->out;
    for (i = 0; i < stages; i++) {
      data->out[i] = a * (data->out[i]) + stage_in - data->in[i];
      data->in[i] = stage_in;
      stage_in = data->out[i];
    }
    break;
  }
  node->out = data->out[i - 1];
}

static inline void
process_bitcrush_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_bitcrush_t *data = &node->data.bitcrush;
  float levels, norm, quantized;

  h_graph_process_node(g, data->input, ctx);
  h_graph_process_node(g, data->target_freq, ctx);
  h_graph_process_node(g, data->bits, ctx);

  data->current_freq += data->target_freq->out;
  if (data->current_freq < ctx->sr) {
    return;
  }
  data->current_freq -= ctx->sr;

  levels = powf(2.0f, data->bits->out);
  if (levels <= 1.0f) {
    node->out = 0.0f;
    return;
  }

  norm = (data->input->out + 1.0f) * 0.5f;
  quantized = floorf(norm * levels) / (levels - 1.0f);
  node->out = quantized * 2.0f - 1.0f;
}

static inline void
process_envelope_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  float time;
  size_t i;
  h_node_envelope_t *data;
  h_graph_node_t *t0, *p0, *t1, *p1;

  time = (ctx->current_frame / ctx->sr) * 1000.0f;

  data = &node->data.envelope;
  for (i = 0; i < data->points.size; i++) {
    p0 = *(h_graph_node_t **) h_vec_get(&data->points, i);
    h_graph_process_node(g, p0, ctx);
  }

  for (size_t i = data->current_idx; i <= data->points.size - 4; i += 2) {
    t0 = *(h_graph_node_t **) h_vec_get(&data->points, i);
    p0 = *(h_graph_node_t **) h_vec_get(&data->points, i + 1);
    t1 = *(h_graph_node_t **) h_vec_get(&data->points, i + 2);
    p1 = *(h_graph_node_t **) h_vec_get(&data->points, i + 3);

    if (time >= t0->out && time < t1->out) {
      node->out = p0->out + (time - t0->out) / (t1->out - t0->out) * (p1->out - p0->out);
      data->current_idx = i;
      return;
    }
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
  case H_NODE_CMP:
    process_cmp_node(g, node, ctx);
    break;
  case H_NODE_CONVERSION:
    process_conversion_node(g, node, ctx);
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
  case H_NODE_CLIP:
    process_hardclip_node(g, node, ctx);
    break;
  case H_NODE_FILTER:
    process_filter_node(g, node, ctx);
    break;
  case H_NODE_BITCRUSH:
    process_bitcrush_node(g, node, ctx);
    break;
  case H_NODE_ENVELOPE:
    process_envelope_node(g, node, ctx);
    break;
  }

  node->last_frame = ctx->current_frame;
}

inline static void
graph_preview(const char *prefix, h_hm_t *g, h_graph_node_t *node, size_t depth)
{
  char margin[32];
  size_t i;

  margin[0] = '|';

  memset(margin + 1, '-', (depth + 1) * 2);
  margin[(depth + 1) * 2] = '\0';
  fprintf(stderr, "%s %s%s ", margin, prefix, node->name);

  switch (node->type) {
  case H_NODE_VALUE:
    fprintf(stderr, "(literal %f)\n", node->out);
    break;
  case H_NODE_MATH:
    fprintf(stderr, "(math, %s)\n", H_OP_MATH[node->data.math.op]);
    for (i = 0; i < node->data.math.values.size; i++) {
      graph_preview("elem:", g, *(h_graph_node_t **) h_vec_get(&node->data.math.values, i), depth + 1);
    }
    break;
  case H_NODE_CMP:
    fprintf(stderr, "(cmp, %s)\n", H_OP_CMP[node->data.cmp.op]);
    graph_preview("left:", g, node->data.cmp.left, depth + 1);
    graph_preview("right:", g, node->data.cmp.right, depth + 1);
    break;
  case H_NODE_CONVERSION:
    fprintf(stderr, "(conversion, %s)\n", H_OP_CONVERSION[node->data.conversion.op]);
    graph_preview("input:", g, node->data.conversion.input, depth + 1);
    break;
  case H_NODE_NOISE:
    fprintf(stderr, "(noise)\n");
    graph_preview("seed:", g, node->data.noise.seed, depth + 1);
    break;
  case H_NODE_OSC:
    fprintf(stderr, "(osc, %s)\n", H_OP_OSC[node->data.osc.type]);
    graph_preview("freq:", g, node->data.osc.freq, depth + 1);
    graph_preview("phase:", g,  node->data.osc.phase, depth + 1);
    break;
  case H_NODE_DIODE:
    fprintf(stderr, "(diode)\n");
    graph_preview("input:", g, node->data.diode, depth + 1);
    break;
  case H_NODE_CLIP:
    fprintf(stderr, "(clip, %s)\n", H_OP_CLIP[node->data.clip.type]);
    graph_preview("input:", g, node->data.clip.input, depth + 1);
    graph_preview("threshold:", g, node->data.clip.threshold, depth + 1);
    break;
  case H_NODE_FILTER:
    fprintf(stderr, "(filter, %s)\n", H_OP_FILTER[node->data.filter.type]);
    graph_preview("input:", g, node->data.filter.input, depth + 1);
    graph_preview("cutoff:", g, node->data.filter.cutoff, depth + 1);
    break;
  case H_NODE_BITCRUSH:
    fprintf(stderr, "(bitcrush)\n");
    graph_preview("input:", g, node->data.bitcrush.input, depth + 1);
    graph_preview("target_freq:", g, node->data.bitcrush.target_freq, depth + 1);
    graph_preview("bits:", g, node->data.bitcrush.bits, depth + 1);
    break;
  case H_NODE_ENVELOPE:
    fprintf(stderr, "(envelope)\n");
    for (i = 0; i < node->data.envelope.points.size; i++) {
      graph_preview("point:", g, *(h_graph_node_t **) h_vec_get(&node->data.envelope.points, i), depth + 1);
    }
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
  h_graph_node_t *node;

  for (i = 0; i < g->capacity; i++) {
    entry = g->buckets[i];
    while (entry) {
      node = entry->value;
      switch (node->type) {
      case H_NODE_MATH:
        h_vec_free(&node->data.math.values);
        break;
      case H_NODE_ENVELOPE:
        h_vec_free(&node->data.envelope.points);
        break;
      default:
        break;
      }
      free(node);
      entry = entry->next;
    }
  }

  h_hm_free(g);
}
