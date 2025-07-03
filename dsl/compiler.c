#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../harsh.h"
#include "../dsl.h"

#define STR_EQN(expected, actual, length) ((!strncmp(expected, actual.p, length) && length == actual.len))
#define STR_EQ(expected, actual) STR_EQN(expected, actual, sizeof(expected) - 1)

static h_graph_node_t *graph_expr_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count);

static inline h_graph_node_t *
graph_expr_from_ast_put(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count)
{
  h_graph_node_t *gn;

  gn = graph_expr_from_ast(g, an, elem_count);
  h_hm_put(g, gn->name, gn);
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

static inline h_graph_node_t *
graph_literal(h_hm_t *g, float value, size_t *elem_count)
{
  h_graph_node_t *res = malloc(sizeof(h_graph_node_t));
  res->type = H_NODE_VALUE;
  res->out = value;
  res->last_frame = 0;
  snprintf(res->name, sizeof(res->name), "_anon_%ld", (*elem_count)++);

  h_hm_put(g, res->name, res);

  return res;
}

static h_graph_node_t *
graph_expr_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count)
{
  h_graph_node_t gn, *inserted, *node, *n_zero, *n_one, **ptr;
  h_dsl_node_t *child;
  char tmp[8], *name;
  int res;
  size_t i;

  snprintf(gn.name, sizeof(gn.name), "_anon_%ld", (*elem_count)++);
  gn.out = 0.0f;
  gn.last_frame = 0;

  if (STR_EQ("ref", an->name)) {
    child = h_vec_get(&an->children, 0);
    memcpy(gn.name, child->name.p, child->name.len);
    gn.name[child->name.len] = '\0';
    inserted = h_hm_get(g, gn.name);
    return inserted;
  } else if (-1 != (res = str_arr_includes(H_OP_MATH, an->name))) {
    gn.type = H_NODE_MATH;
    gn.data.math.op = res;
    h_vec_init(&gn.data.math.values, 2, sizeof(h_graph_node_t **));
    for (i = 0; i < an->children.size; i++) {
      node = graph_expr_from_ast_put(g, h_vec_get(&an->children, i), elem_count);
      h_vec_push(&gn.data.math.values, &node);
    }
  } else if (-1 != (res = str_arr_includes(H_OP_CMP, an->name))) {
    gn.type = H_NODE_CMP;
    gn.data.cmp.op = res;
    gn.data.cmp.left = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
    gn.data.cmp.right = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count);
  } else if (-1 != (res = str_arr_includes(H_OP_CONVERSION, an->name))) {
    gn.type = H_NODE_CONVERSION;
    gn.data.conversion.op = res;
    gn.data.conversion.input = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
  } else if (STR_EQ("noise", an->name)) {
    gn.type = H_NODE_NOISE;
    for (i = 0; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":seed", child->name)) {
        gn.data.noise.state = 1;
        gn.data.noise.seed = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count);
      }
    }
  } else if (-1 != (res = str_arr_includes(H_OP_OSC, an->name))) {
    gn.type = H_NODE_OSC;
    gn.data.osc.type = res;
    gn.data.osc.current = 0.0f;
    gn.data.osc.freq = 0;
    gn.data.osc.phase = 0;
    for (i = 0; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":freq", child->name)) {
        ptr = &gn.data.osc.freq;
      } else if (STR_EQ(":phase", child->name)) {
        ptr = &gn.data.osc.phase;
      } else {
        fprintf(stderr, "unexpected argument for osc: '%.*s'\n", (int) child->plain.len - 1, child->plain.p + 1);
        continue;
      }
      *ptr = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count);
    }
    if (0 == gn.data.osc.phase) {
      gn.data.osc.phase = graph_literal(g, 0.0, elem_count);
    }
  } else if (STR_EQ("diode", an->name)) {
    gn.type = H_NODE_DIODE;
    gn.data.diode = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
  } else if (-1 != (res = str_arr_includes(H_OP_CLIP, an->name))) {
    gn.type = H_NODE_CLIP;
    gn.data.clip.type = res;
    gn.data.clip.threshold = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
    gn.data.clip.input = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count);
  } else if (-1 != (res = str_arr_includes(H_OP_FILTER, an->name))) {
    gn.type = H_NODE_FILTER;
    gn.data.filter.type = res;
    for (i = 0; i < 4; i++) {
      gn.data.filter.in[i] = 0.0f;
    }
    for (i = 0; i < 4; i++) {
      gn.data.filter.out[i] = 0.0f;
    }
    gn.data.filter.cutoff = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
    gn.data.filter.stages = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count);
    gn.data.filter.input = graph_expr_from_ast_put(g, h_vec_get(&an->children, 2), elem_count);
  } else if (STR_EQ("bitcrush", an->name)) {
    child = h_vec_get(&an->children, 0);
    gn.type = H_NODE_BITCRUSH;
    gn.data.bitcrush.current_freq = 0.0f;
    gn.data.bitcrush.input = graph_expr_from_ast_put(g, child, elem_count);
    for (i = 1; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":target_freq", child->name)) {
        ptr = &gn.data.bitcrush.target_freq;
      } else if (STR_EQ(":bits", child->name)) {
        ptr = &gn.data.bitcrush.bits;
      } else {
        fprintf(stderr, "unexpected argument for bitcrush: '%.*s'\n", (int) child->plain.len - 1, child->plain.p + 1);
        continue;
      }
      *ptr = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count);
    }
  } else if (STR_EQ("envelope", an->name)) {
    gn.type = H_NODE_ENVELOPE;
    gn.data.envelope.current_idx = 0;
    h_vec_init(&gn.data.envelope.points, 8, sizeof(h_graph_node_t **));
    for (i = 0; i < an->children.size; i++) {
      node = graph_expr_from_ast_put(g, h_vec_get(&an->children, i), elem_count);
      h_vec_push(&gn.data.envelope.points, &node);
    }
  } else if (STR_EQ("ad", an->name)) {
    gn.type = H_NODE_ENVELOPE;
    gn.data.envelope.current_idx = 0;
    h_vec_init(&gn.data.envelope.points, 6, sizeof(h_graph_node_t **));
    n_zero = graph_literal(g, 0.0, elem_count);
    n_one = graph_literal(g, 1.0, elem_count);
    h_vec_push(&gn.data.envelope.points, &n_zero);
    h_vec_push(&gn.data.envelope.points, &n_zero);
    node = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count);
    h_vec_push(&gn.data.envelope.points, &node);
    h_vec_push(&gn.data.envelope.points, &n_one);
    node = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count);
    h_vec_push(&gn.data.envelope.points, &node);
    h_vec_push(&gn.data.envelope.points, &n_zero);
  } else {
    gn.type = H_NODE_VALUE;
    memcpy(tmp, an->name.p, an->name.len);
    tmp[an->name.len] = '\0';
    gn.out = strtof(tmp, 0);
  }

  inserted = malloc(sizeof(h_graph_node_t));
  memcpy(inserted, &gn, sizeof(h_graph_node_t));
  return inserted;
}

static void
graph_from_ast(h_hm_t *g, h_dsl_node_t *an, size_t *elem_count)
{
  h_graph_node_t *gn;
  h_dsl_node_t *child;

  if (STR_EQ("def", an->name)) {
    child = h_vec_get(&an->children, 0);
    gn = graph_expr_from_ast(g, h_vec_get(&an->children, 1), elem_count);
    memcpy(gn->name, child->name.p, child->name.len);
    gn->name[child->name.len] = '\0';
    h_hm_put(g, gn->name, gn);
  } else {
    graph_expr_from_ast_put(g, an, elem_count);
  }
}

void
h_dsl_load(h_hm_t *g, const char *src, size_t src_len)
{
  h_dsl_node_t root;
  size_t elem_count = 0, i;

  h_hm_init(g, 16, 0.75f, h_hash_string, h_eq_string);

  h_dsl_parse(&root, src, src_len);
  for (i = 0; i < root.children.size; i++) {
    graph_from_ast(g, h_vec_get(&root.children, i), &elem_count);
  }
  h_dsl_free_node(&root);
}
