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
  size_t i, j, k;
  h_graph_node_t *elem;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      k = 0;

      switch (data.op) {
      case H_NODE_MATH_ADD:
        node->out[i][j] = 0.0f;
        break;
      case H_NODE_MATH_MUL:
        node->out[i][j] = 1.0f;
        break;
      case H_NODE_MATH_DIV:
      case H_NODE_MATH_POW:
        elem = *(h_graph_node_t **) h_vec_get(&data.values, k++);
        h_graph_process_node(g, elem, ctx);
        node->out[i][j] = elem->out[i][j];
        break;
      case H_NODE_MATH_SUB:
        if (data.values.size > 1) {
          elem = *(h_graph_node_t **) h_vec_get(&data.values, k++);
          h_graph_process_node(g, elem, ctx);
          node->out[i][j] = elem->out[i][j];
        } else {
          node->out[i][j] = 0.0f;
        }
        break;
      default:
        break;
      }

      for (; k < data.values.size; k++) {
        elem = *(h_graph_node_t **) h_vec_get(&data.values, k);
        h_graph_process_node(g, elem, ctx);
        switch (data.op) {
        case H_NODE_MATH_ADD:
          node->out[i][j] += elem->out[i][j];
          break;
        case H_NODE_MATH_SUB:
          node->out[i][j] -= elem->out[i][j];
          break;
        case H_NODE_MATH_MUL:
          node->out[i][j] *= elem->out[i][j];
          break;
        case H_NODE_MATH_DIV:
          node->out[i][j] = 0.0f == elem->out[i][j] ? 0.0f : node->out[i][j] / elem->out[i][j];
          break;
        case H_NODE_MATH_POW:
          node->out[i][j] = powf(node->out[i][j], elem->out[i][j]);
          break;
        case H_NODE_MATH_LOGN:
          node->out[i][j] = elem->out[i][j] < 0.0f ? 0.0f : logf(elem->out[i][j]);
          break;
        case H_NODE_MATH_LOG2:
          node->out[i][j] = elem->out[i][j] < 0.0f ? 0.0f : log2f(elem->out[i][j]);
          break;
        case H_NODE_MATH_LOG10:
          node->out[i][j] = elem->out[i][j] < 0.0f ? 0.0f : log10f(elem->out[i][j]);
          break;
        case H_NODE_MATH_EXP:
          node->out[i][j] = expf(elem->out[i][j]);
          break;
        }
      }
    }
  }
}

static inline void
process_cmp_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_cmp_t data = node->data.cmp;
  size_t i, j;
  char res;

  h_graph_process_node(g, data.left, ctx);
  h_graph_process_node(g, data.right, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      switch (data.op) {
      case H_NODE_CMP_LT:
        res = data.left->out[i][j] < data.right->out[i][j];
        break;
      case H_NODE_CMP_LEQT:
        res = data.left->out[i][j] <= data.right->out[i][j];
        break;
      case H_NODE_CMP_GT:
        res = data.left->out[i][j] > data.right->out[i][j];
        break;
      case H_NODE_CMP_GEQT:
        res = data.left->out[i][j] >= data.right->out[i][j];
        break;
      case H_NODE_CMP_EQ:
        res = data.left->out[i][j] == data.right->out[i][j];
        break;
      case H_NODE_CMP_NEQ:
        res = data.left->out[i][j] != data.right->out[i][j];
        break;
      }

      node->out[i][j] = res ? 1.0f : 0.0f;
    }
  }
}

static inline void
process_conversion_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_conversion_t data = node->data.conversion;
  size_t i, j;

  h_graph_process_node(g, data.input, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      switch (data.op) {
      case H_NODE_CONVERSION_MTOF:
        node->out[i][j] = 440.0f * powf(2.0f, (data.input->out[i][j] - 69.0f) / 12.0f);
        break;
      case H_NODE_CONVERSION_FTOM:
        node->out[i][j] = 69.0f + 12.0f * log2f(data.input->out[i][j] / 440.0f);
        break;
      case H_NODE_CONVERSION_DTOA:
        node->out[i][j] = 20.0f * log10f(data.input->out[i][j]);
        break;
      case H_NODE_CONVERSION_ATOD:
        node->out[i][j] = powf(10.0f, node->out[i][j] / 20.0f);
        break;
      }
    }
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
  size_t i, j;

  h_graph_process_node(g, data->seed, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      data->state[i] ^= (unsigned int) data->seed->out[i][j];

      float u1 = ((float) xorshift32(&data->state[i]) + 1.0f) / ((float) UINT32_MAX + 2.0f);
      float u2 = ((float) xorshift32(&data->state[i]) + 1.0f) / ((float) UINT32_MAX + 2.0f);
      node->out[i][j] = sqrtf(-2.0f * logf(u1)) * cosf(2.0f * M_PI * u2);
    }
  }
}

static inline void
process_osc_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_osc_t *data = &node->data.osc;
  size_t i, j;

  h_graph_process_node(g, data->freq, ctx);
  h_graph_process_node(g, data->phase, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      switch (data->type) {
      case H_NODE_OSC_SINE:
        node->out[i][j] = sinf(data->current[i] + data->phase->out[i][j]);
        data->current[i] += 2.0f * M_PI * data->freq->out[i][j] / ctx->sr;
        data->current[i] = fmod(data->current[i], 2.0f * M_PI);
        if (data->current[i] < 0.0f) data->current[i] += 2.0f * M_PI;
        break;
      case H_NODE_OSC_SQUARE:
        node->out[i][j] = (data->current[i] + data->phase->out[i][j] < 0.5 ? -1.0f : 1.0f);
        data->current[i] += data->freq->out[i][j] / ctx->sr;
        data->current[i] = fmodf(data->current[i], 1.0f);
        if (data->current[i] < 0.0f) data->current[i] += 1.0f;
        break;
      case H_NODE_OSC_SAWTOOTH:
        node->out[i][j] = (1.0f - 2.0f + data->phase->out[i][j]);
        data->current[i] += data->freq->out[i][j] / ctx->sr;
        data->current[i] = fmodf(data->current[i], 1.0f);
        if (data->current[i] < 0.0f) data->current[i] += 1.0f;
        break;
      }
    }
  }
}

static inline void
process_diode_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  size_t i;

  h_graph_process_node(g, node->data.diode, ctx);

  for (i = 0; i < BLOCK_SIZE; i++) {
    node->out[0][i] = log1pf(expf(node->data.diode->out[0][i]));
    node->out[1][i] = log1pf(expf(node->data.diode->out[1][i]));
  }
}

static inline void
process_hardclip_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_clip_t data = node->data.clip;
  size_t i, j;

  h_graph_process_node(g, data.threshold, ctx);
  h_graph_process_node(g, data.input, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      switch (data.type) {
      case H_NODE_CLIP_HARDCLIP:
        node->out[i][j] = fminf(fmaxf(data.input->out[i][j], -data.threshold->out[i][j]), data.threshold->out[i][j]);
        break;
      case H_NODE_CLIP_FOLDBACK:
        /* https://www.musicdsp.org/en/latest/Effects/203-fold-back-distortion.html */
        if (data.input->out[i][j] > data.threshold->out[i][j] || data.input->out[i][j] < -data.threshold->out[i][j]) {
          node->out[i][j] = fabsf(fabsf(fmodf(data.input->out[i][j], data.threshold->out[i][j] * 4)) - data.threshold->out[i][j] * 2) - data.threshold->out[i][j];
        } else {
          node->out[i][j] = data.input->out[i][j];
        }
        break;
      }
    }
  }
}

static inline void
process_filter_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_filter_t *data;
  float a, stage_in;
  size_t i, j, k;
  int stages;

  data = &node->data.filter;

  h_graph_process_node(g, data->input, ctx);
  h_graph_process_node(g, data->stages, ctx);
  h_graph_process_node(g, data->cutoff, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      a = expf(-2.0f * M_PI * data->cutoff->out[i][j] / ctx->sr);
      stages = (int) roundf(fmax(1.0, fmin(4.0, data->stages->out[i][j])));
      switch (data->type) {
      case H_NODE_FILTER_LOWPASS:
        stage_in = data->input->out[i][j];
        for (k = 0; k < stages; k++) {
          data->out[i][k] = (1 - a) * stage_in + a * data->out[i][k];
          stage_in = data->out[i][k];
        }
        break;
      case H_NODE_FILTER_HIGHPASS:
        stage_in = data->input->out[i][j];
        for (k = 0; k < stages; k++) {
          data->out[i][k] = a * (data->out[i][k]) + stage_in - data->in[i][k];
          data->in[i][k] = stage_in;
          stage_in = data->out[i][k];
        }
        break;
      }
      node->out[i][j] = data->out[i][k - 1];
    }
  }
}

static inline void
process_bitcrush_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_bitcrush_t *data = &node->data.bitcrush;
  size_t i, j;
  float levels, norm, quantized;

  h_graph_process_node(g, data->input, ctx);
  h_graph_process_node(g, data->target_freq, ctx);
  h_graph_process_node(g, data->bits, ctx);

  for (i = 0; i < 2; i++) {
    for (j = 0; j < BLOCK_SIZE; j++) {
      data->current_freq[i] += data->target_freq->out[i][j];
      if (data->current_freq[i] < ctx->sr) {
        node->out[i][j] = data->held[i];
        continue;
      }
      data->current_freq[i] -= ctx->sr;

      levels = powf(2.0f, data->bits->out[i][j]);
      if (levels <= 1.0f) {
        node->out[i][j] = 0.0f;
        continue;
      }

      norm = (data->input->out[i][j] + 1.0f) * 0.5f;
      quantized = floorf(norm * levels) / (levels - 1.0f);
      data->held[i] = quantized * 2.0f - 1.0f;
      node->out[i][j] = data->held[i];
    }
  }
}

static inline void
process_envelope_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  float time;
  size_t i, j, k;
  h_node_envelope_t *data;
  h_graph_node_t *t0, *p0, *t1, *p1;

  data = &node->data.envelope;
  for (j = 0; j < data->points.size; j++) {
    p0 = *(h_graph_node_t **) h_vec_get(&data->points, j);
    h_graph_process_node(g, p0, ctx);
  }

  for (i = 0; i < 2; i++) {
    for (j = data->current_idx; j <= data->points.size - 4; j += 2) {
      for (k = 0; k < BLOCK_SIZE; k++) {
        time = (ctx->current_block * BLOCK_SIZE / ctx->sr) * 1000.0f;

        t0 = *(h_graph_node_t **) h_vec_get(&data->points, j);
        p0 = *(h_graph_node_t **) h_vec_get(&data->points, j + 1);
        t1 = *(h_graph_node_t **) h_vec_get(&data->points, j + 2);
        p1 = *(h_graph_node_t **) h_vec_get(&data->points, j + 3);

        if (time >= t0->out[i][k] && time < t1->out[i][k]) {
          node->out[i][k] = p0->out[i][k] + (time - t0->out[i][k]) / (t1->out[i][k] - t0->out[i][k]) * (p1->out[i][k] - p0->out[i][k]);
          data->current_idx = j;
          continue;
        }
      }
    }
  }
}

static inline void
process_pan_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_pan_t *pan = &node->data.pan;
  size_t i;

  h_graph_process_node(g, pan->input, ctx);
  h_graph_process_node(g, pan->alpha, ctx);

  for (i = 0; i < BLOCK_SIZE; i++) {
    node->out[0][i] = pan->input->out[0][i] * (1 - fmaxf(0, pan->alpha->out[0][i]));
    node->out[1][i] = pan->input->out[1][i] * (1 + fminf(0, pan->alpha->out[0][i]));
  }
}

#ifndef __EMSCRIPTEN__
static inline void
process_audio_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  h_node_audio_t *data = &node->data.audio;
  size_t i, j;

  h_graph_process_node(g, data->length, ctx);

  for (i = 0; i < BLOCK_SIZE; i++) {
    for (j = 0; j < 2; j++) {
      if (data->current_sample >= data->length->out[j][i] * data->sample_rate) {
        node->out[j][i] = 0.0f;
        continue;
      }

      data->current_freq += data->sample_rate;
      if (data->current_freq < ctx->sr) {
        node->out[j][i] = 0.0f;
        continue;
      }

      data->held[j] = data->samples[data->current_sample * data->channel_count + j];
      node->out[j][i] = data->held[j];
    }

    data->current_sample++;
  }
}
#endif

void
h_graph_process_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx)
{
  if (node->last_block != ctx->current_block) {
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
  case H_NODE_PAN:
    process_pan_node(g, node, ctx);
    break;
  case H_NODE_ENVELOPE:
    process_envelope_node(g, node, ctx);
    break;
  #ifndef __EMSCRIPTEN__
  case H_NODE_AUDIO:
    process_audio_node(g, node, ctx);
    break;
  #endif
  }

  node->last_block++;
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
    fprintf(stderr, "(literal %f)\n", node->out[0][0]);
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
  case H_NODE_PAN:
    fprintf(stderr, "(pan)\n");
    graph_preview("input:", g, node->data.pan.input, depth + 1);
    graph_preview("alpha:", g, node->data.pan.alpha, depth + 1);
    break;
  case H_NODE_ENVELOPE:
    fprintf(stderr, "(envelope)\n");
    for (i = 0; i < node->data.envelope.points.size; i++) {
      graph_preview("point:", g, *(h_graph_node_t **) h_vec_get(&node->data.envelope.points, i), depth + 1);
    }
    break;
  #ifndef __EMSCRIPTEN__
  case H_NODE_AUDIO:
    fprintf(stderr, "(audio)\n");
    graph_preview("length:", g, node->data.audio.length, depth + 1);
    break;
  #endif
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
      #ifndef __EMSCRIPTEN__
      case H_NODE_AUDIO:
        free(node->data.audio.samples);
        break;
      #endif
      default:
        break;
      }
      free(node);
      entry = entry->next;
    }
  }

  h_hm_free(g);
}
