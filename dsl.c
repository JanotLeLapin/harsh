#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harsh.h"

#define STR_EQ(expected, actual) (!strncmp(expected, actual.p, sizeof(expected) - 1) && sizeof(expected) - 1 == actual.len)

typedef struct {
  const char *src;
  size_t src_len;
  size_t i;
} parser_ctx_t;

typedef struct {
  const char *p;
  size_t len;
} src_string_t;

typedef struct ast_node_s {
  src_string_t name;
  size_t child_count;
  struct ast_node_s *children;
} ast_node_t;

static inline char
is_whitespace(char c)
{
  switch (c) {
  case ' ':
  case '\n':
  case '\t':
    return 1;
  default:
    return 0;
  }
}

static void
parse_node(ast_node_t *node, parser_ctx_t *ctx)
{
  ast_node_t *child;

  while (is_whitespace(ctx->src[ctx->i])) {
    ctx->i++;
  }
  ctx->i++;
  node->name.p = ctx->src + ctx->i;
  while (!is_whitespace(ctx->src[ctx->i])) {
    ctx->i++;
  }
  node->name.len = ctx->src + ctx->i - node->name.p;

  node->children = malloc(sizeof(ast_node_t) * 8);
  node->child_count = 0;

  for (;;) {
    while (is_whitespace(ctx->src[ctx->i])) {
      ctx->i++;
    }

    switch (ctx->src[ctx->i]) {
    case ')':
      ctx->i++;
      return;
    case '(':
      parse_node(&node->children[node->child_count], ctx);
      node->child_count++;
      continue;
    default:
      break;
    }

    child = &node->children[node->child_count];
    child->name.p = ctx->src + ctx->i;
    while (!is_whitespace(ctx->src[ctx->i]) && ')' != ctx->src[ctx->i]) {
      ctx->i++;
    }
    child->name.len = ctx->src + ctx->i - child->name.p;

    child->children = 0;
    child->child_count = 0;

    node->child_count++;
  }
}

static void
free_node(ast_node_t *node)
{
  size_t i;

  for (i = 0; i < node->child_count; i++) {
    free_node(&node->children[i]);
  }

  if (0 != node->children) {
    free(node->children);
  }
}

static h_graph_node_t *graph_expr_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count);

static inline h_graph_node_t *
graph_expr_from_ast_put(h_hm_t *g, ast_node_t *an, size_t *elem_count)
{
  h_graph_node_t *gn;

  gn = graph_expr_from_ast(g, an, elem_count);
  h_hm_put(g, gn->name, gn);
  return gn;
}

static h_graph_node_t *
graph_expr_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count)
{
  h_graph_node_t gn, *inserted;
  char tmp[8], **ptr;
  size_t i;

  snprintf(gn.name, sizeof(gn.name), "_anon_%ld", (*elem_count)++);
  gn.out = 0.0f;
  gn.last_frame = 0;

  if (STR_EQ("ref", an->name)) {
    memcpy(gn.name, an->children[0].name.p, an->children[0].name.len);
    gn.name[an->children[0].name.len] = '\0';
    inserted = h_hm_get(g, gn.name);
    return inserted;
  } else if (STR_EQ("*", an->name)) {
    gn.type = H_NODE_MATH;
    gn.data.math.op = H_NODE_MATH_MUL;
    gn.data.math.left = graph_expr_from_ast_put(g, &an->children[0], elem_count)->name;
    gn.data.math.right = graph_expr_from_ast_put(g, &an->children[1], elem_count)->name;
  } else if (STR_EQ("noise", an->name)) {
    gn.type = H_NODE_NOISE;
    for (i = 0; i < an->child_count; i += 2) {
      if (STR_EQ(":seed", an->children[i].name)) {
        gn.data.noise.state = 1;
        gn.data.noise.seed = graph_expr_from_ast_put(g, &an->children[i + 1], elem_count)->name;
      }
    }
  } else if (STR_EQ("sine", an->name)) {
    gn.type = H_NODE_OSC;
    gn.data.osc.type = H_NODE_OSC_SINE;
    gn.data.osc.current = 0.0f;
    for (i = 0; i < an->child_count; i += 2) {
      if (STR_EQ(":freq", an->children[i].name)) {
        ptr = &gn.data.osc.freq;
      } else if (STR_EQ(":phase", an->children[i].name)) {
        ptr = &gn.data.osc.phase;
      } else {
        continue;
      }
      *ptr = graph_expr_from_ast_put(g, &an->children[i + 1], elem_count)->name;
    }
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
graph_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count)
{
  h_graph_node_t *gn;

  if (STR_EQ("def", an->name)) {
    gn = graph_expr_from_ast(g, &an->children[1], elem_count);
    memcpy(gn->name, an->children[0].name.p, an->children[0].name.len);
    gn->name[an->children[0].name.len] = '\0';
    h_hm_put(g, gn->name, gn);
  } else {
    graph_expr_from_ast_put(g, an, elem_count);
  }
}

void
h_dsl_load(h_hm_t *g, const char *src, size_t src_len)
{
  ast_node_t root;
  size_t elem_count = 0;
  parser_ctx_t ctx = { .i = 0, .src = src, .src_len = src_len };
  size_t i;

  h_hm_init(g, 16, 0.75f, h_hash_string, h_eq_string);

  parse_node(&root, &ctx);
  for (i = 0; i < root.child_count; i++) {
    graph_from_ast(g, &root.children[i], &elem_count);
  }
  free_node(&root);
}
