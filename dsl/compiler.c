#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __EMSCRIPTEN__
#include <sndfile.h>
#endif

#include "../harsh.h"

#define STR_EQN(expected, actual, length) ((!strncmp(expected, actual.p, length) && length == actual.len))
#define STR_EQ(expected, actual) STR_EQN(expected, actual, sizeof(expected) - 1)

#define ARG(aname, arequired, adef, atarget) (arg_spec_t) { .name = aname, .required = arequired, .def = adef, .target = atarget }
#define ARG_OPT(aname, adef, atarget) ARG(aname, 0, adef, atarget)
#define ARG_REQ(aname, atarget) ARG(aname, 1, 0.0f, atarget)

typedef struct {
  const char *name;
  char required;
  float def;
  h_graph_node_t **target;
} arg_spec_t;

typedef struct {
  enum {
    ARG_SPECS_VARARG,
    ARG_SPECS_TYPED,
  } type;
  union {
    struct {
      arg_spec_t args[DSL_MAX_ARGS];
      size_t count;
    };
    h_vec_t *target;
  };
} arg_specs_t;

static h_graph_node_t *graph_expr_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count, h_dsl_ctx_t *ctx);

static inline h_graph_node_t *
graph_expr_from_ast_put(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count, h_dsl_ctx_t *ctx)
{
  h_graph_node_t *gn;

  gn = graph_expr_from_ast(g, an, elem_count, ctx);
  if (0 != gn) {
    h_hm_put(g, gn->name, gn);
  }
  return gn;
}

static inline int
str_arr_includes(const char **expected, h_dsl_string_t str)
{
  size_t i;

  for (i = 0;; i++) {
    if (0 == expected[i]) {
      break;
    }

    if (STR_EQN(expected[i], str, strlen(expected[i]))) {
      return i;
    }
  }

  return -1;
}

#ifndef __EMSCRIPTEN__
static inline int
load_audio(h_node_audio_t *data, const char *filename)
{
  SF_INFO sfinfo = {0};
  SNDFILE *f;
  sf_count_t read_count;

  f = sf_open(filename, SFM_READ, &sfinfo);
  if (0 == f) {
    fprintf(stderr, "sf_open: %s\n", sf_strerror(NULL));
    return -1;
  }

  data->sample_rate = (float) sfinfo.samplerate;
  data->sample_count = (size_t) sfinfo.frames * sfinfo.channels;
  data->channel_count = (size_t) sfinfo.channels;
  data->samples = malloc(sizeof(float) * data->sample_count);
  if (0 == data->samples) {
    perror("malloc");
    sf_close(f);
    return -1;
  }

  read_count = sf_readf_float(f, data->samples, sfinfo.frames);
  sf_close(f);
  if (read_count != sfinfo.frames) {
    fprintf(stderr, "sf_readf_float: incomplete read\n");
    return -1;
  }

  data->current_sample = 0;
  data->current_freq = 0.0f;
  return 0;
}
#endif

static inline int
arg_spec(arg_specs_t *specs, const h_dsl_string_t str, h_graph_node_t *node)
{
  int res;
  size_t i;

  if (-1 != (res = str_arr_includes(H_OP_MATH, str))) {
    node->type = H_NODE_MATH;
    node->data.math.op = res;
    specs->type = ARG_SPECS_VARARG;
    specs->target = &node->data.math.values;
    return 1;
  } else if (-1 != (res = str_arr_includes(H_OP_CMP, str))) {
    node->type = H_NODE_CMP;
    node->data.cmp.op = res;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":left", &node->data.cmp.left);
    specs->args[1] = ARG_REQ(":right", &node->data.cmp.right);
    specs->count = 2;
    return 1;
  } else if (-1 != (res = str_arr_includes(H_OP_CONVERSION, str))) {
    node->type = H_NODE_CONVERSION;
    node->data.conversion.op = res;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.conversion.input);
    specs->count = 1;
    return 1;
  } else if (STR_EQ("noise", str)) {
    node->type = H_NODE_NOISE;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_OPT(":seed", 0.0f, &node->data.noise.seed);
    specs->count = 1;
    return 1;
  } else if (-1 != (res = str_arr_includes(H_OP_OSC, str))) {
    node->type = H_NODE_OSC;
    node->data.osc.type = res;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":freq", &node->data.osc.freq);
    specs->args[1] = ARG_OPT(":phase", 0.0f, &node->data.osc.phase);
    specs->count = 2;
    return 1;
  } else if (STR_EQ("diode", str)) {
    node->type = H_NODE_DIODE;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.diode);
    specs->count = 1;
    return 1;
  } else if (-1 != (res = str_arr_includes(H_OP_CLIP, str))) {
    node->type = H_NODE_CLIP;
    node->data.clip.type = res;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.clip.input);
    specs->args[1] = ARG_REQ(":threshold", &node->data.clip.threshold);
    specs->count = 2;
    return 1;
  } else if (-1 != (res = str_arr_includes(H_OP_FILTER, str))) {
    node->type = H_NODE_FILTER;
    node->data.filter.type = res;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.filter.input);
    specs->args[1] = ARG_REQ(":cutoff", &node->data.filter.cutoff);
    specs->args[2] = ARG_OPT(":stages", 1.0f, &node->data.filter.stages);
    specs->count = 3;
    return 1;
  } else if (STR_EQ("bitcrush", str)) {
    node->type = H_NODE_BITCRUSH;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.bitcrush.input);
    specs->args[1] = ARG_OPT(":target_freq", 44100.0f, &node->data.bitcrush.target_freq);
    specs->args[2] = ARG_OPT(":bits", 16.0f, &node->data.bitcrush.bits);
    specs->count = 3;
    return 1;
  } else if (STR_EQ("pan", str)) {
    node->type = H_NODE_PAN;
    specs->type = ARG_SPECS_TYPED;
    specs->args[0] = ARG_REQ(":in", &node->data.pan.input);
    specs->args[1] = ARG_OPT(":alpha", 0.0, &node->data.pan.alpha);
    specs->count = 2;
    return 1;
  } else if (STR_EQ("envelope", str)) {
    node->type = H_NODE_ENVELOPE;
    node->data.envelope.current_idx = 0;
    specs->type = ARG_SPECS_VARARG;
    specs->target = &node->data.envelope.points;
    return 1;
  }

  return 0;
}

static inline h_graph_node_t *
graph_literal(h_hm_t *g, float value, size_t *elem_count)
{
  h_graph_node_t *res = malloc(sizeof(h_graph_node_t));
  size_t i;

  res->type = H_NODE_VALUE;
  for (i = 0; i < BLOCK_SIZE; i++) {
    res->out[0][i] = value;
    res->out[1][i] = value;
  }
  res->last_block = 0;
  snprintf(res->name, sizeof(res->name), "_anon_%ld", (*elem_count)++);

  h_hm_put(g, res->name, res);

  return res;
}

static h_graph_node_t *
graph_expr_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count, h_dsl_ctx_t *ctx)
{
  h_graph_node_t gn, *inserted, *node, *n_zero, *n_one;
  h_dsl_node_t *child;
  char tmp[64];
  int res;
  arg_specs_t specs;
  size_t i, j, current = 0;
  char found;
  h_dsl_error_t *err;

  snprintf(gn.name, sizeof(gn.name), "_anon_%ld", (*elem_count)++);
  gn.last_block = 0;
  memset(gn.out, 0, BLOCK_SIZE * 2);
  memset(&gn.data, 0, sizeof(h_graph_node_data_t));

  if (STR_EQ("ref", an->name)) {
    child = h_vec_get(&an->children, 0);
    memcpy(gn.name, child->name.p, child->name.len);
    gn.name[child->name.len] = '\0';
    inserted = h_hm_get(g, gn.name);
    if (0 == inserted) {
      ctx->failed = 1;
      if (ctx->error_count < 16) {
        err = &ctx->errors[ctx->error_count++];
        err->type = H_DSL_ERROR_SEVERE;
        err->message = "unknown node referenced";
        err->node = *an;
        err->problem = *((h_dsl_node_t *) h_vec_get(&an->children, 0));
      }
    }
    return inserted;
  } else if (STR_EQ("ad", an->name)) {
    gn.type = H_NODE_ENVELOPE;
    gn.data.envelope.current_idx = 0;
    h_vec_init(&gn.data.envelope.points, 6, sizeof(h_graph_node_t **));
    n_zero = graph_literal(g, 0.0, elem_count);
    n_one = graph_literal(g, 1.0, elem_count);
    h_vec_push(&gn.data.envelope.points, &n_zero);
    h_vec_push(&gn.data.envelope.points, &n_zero);
    node = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count, ctx);
    h_vec_push(&gn.data.envelope.points, &node);
    h_vec_push(&gn.data.envelope.points, &n_one);
    node = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count, ctx);
    h_vec_push(&gn.data.envelope.points, &node);
    h_vec_push(&gn.data.envelope.points, &n_zero);
  }
  #ifndef __EMSCRIPTEN__
  else if (STR_EQ("audio", an->name)) {
    gn.type = H_NODE_AUDIO;
    child = h_vec_get(&an->children, 0);
    memcpy(tmp, child->plain.p, child->plain.len);
    tmp[child->plain.len - 1] = '\0';
    load_audio(&gn.data.audio, tmp + 1);
    gn.data.audio.length = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count, ctx);
  }
  #endif
  else if (1 == arg_spec(&specs, an->name, &gn)) {
    switch (specs.type) {
    case ARG_SPECS_VARARG:
      h_vec_init(specs.target, 8, sizeof(h_graph_node_t **));
      for (i = 0; i < an->children.size; i++) {
        node = graph_expr_from_ast_put(g, h_vec_get(&an->children, i), elem_count, ctx);
        if (0 != node) {
          h_vec_push(specs.target, &node);
        }
      }
      break;
    case ARG_SPECS_TYPED:
      for (i = 0; i < an->children.size; i++) {
        child = h_vec_get(&an->children, i);
        if (':' == child->name.p[0]) {
          found = 0;
          for (j = 0; j < specs.count; j++) {
            if (!strncmp(specs.args[j].name, child->name.p, child->name.len) && specs.args[j].name[child->name.len] == '\0') {
              child = h_vec_get(&an->children, ++i);
              *specs.args[j].target = graph_expr_from_ast_put(g, child, elem_count, ctx);
              found = 1;
              break;
            }
          }

          if (!found && ctx->error_count < DSL_MAX_ERRORS) {
            err = &ctx->errors[ctx->error_count++];
            err->type = H_DSL_ERROR_WARN;
            err->node = *an;
            err->problem = *child;
            err->message = "unrecognized argument";
          }
        } else {
          *specs.args[current++].target = graph_expr_from_ast_put(g, child, elem_count, ctx);
        }
      }

      for (i = 0; i < specs.count; i++) {
        if (0 == *specs.args[i].target) {
          if (specs.args[i].required) {
            ctx->failed = 1;
            if (ctx->error_count < DSL_MAX_ERRORS) {
              err = &ctx->errors[ctx->error_count++];
              err->type = H_DSL_ERROR_SEVERE;
              err->node = *an;
              err->problem = *an;
              err->message = "missing required arg";
            }
          } else {
            *specs.args[i].target = graph_literal(g, specs.args[i].def, elem_count);
          }
        }
      }
      break;
    }
  } else {
    gn.type = H_NODE_VALUE;
    memcpy(tmp, an->name.p, an->name.len);
    tmp[an->name.len] = '\0';
    gn.out[0][0] = strtof(tmp, 0);
    for (i = 0; i < BLOCK_SIZE; i++) {
      gn.out[0][i] = gn.out[0][0];
      gn.out[1][i] = gn.out[0][0];
    }
  }

  inserted = malloc(sizeof(h_graph_node_t));
  memcpy(inserted, &gn, sizeof(h_graph_node_t));
  return inserted;
}

static void
graph_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count, h_dsl_ctx_t *ctx)
{
  h_graph_node_t *gn;
  h_dsl_node_t *child;

  if (STR_EQ("def", an->name)) {
    child = h_vec_get(&an->children, 0);
    gn = graph_expr_from_ast(g, h_vec_get(&an->children, 1), elem_count, ctx);
    if (0 == gn) {
      return;
    }
    memcpy(gn->name, child->name.p, child->name.len);
    gn->name[child->name.len] = '\0';
    h_hm_put(g, gn->name, gn);
  } else {
    graph_expr_from_ast_put(g, an, elem_count, ctx);
  }
}

void
h_dsl_load(h_hm_t *g, h_dsl_ctx_t *ctx)
{
  h_dsl_node_t root;
  size_t elem_count = 0, i;
  h_dsl_error_t *err;
  char *label;

  h_hm_init(g, 16, 0.75f, h_hash_string, h_eq_string);

  h_dsl_parse(&root, ctx->src, ctx->src_len);
  for (i = 0; i < root.children.size; i++) {
    graph_from_ast(g, h_vec_get(&root.children, i), &elem_count, ctx);
  }
  h_dsl_free_node(&root);

  if (!ctx->failed) {
    h_dsl_optimize(g);
  }
}
