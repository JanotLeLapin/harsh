#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harsh.h"

#define STR_EQN(expected, actual, length) ((!strncmp(expected, actual.p, length) && length == actual.len))
#define STR_EQ(expected, actual) STR_EQN(expected, actual, sizeof(expected) - 1)

#define COLOR_RESET "\x1b[0m"
#define COLOR_RED "\x1b[31m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_BLUE "\x1b[36m"

static const char *OP_MATH[] = { "+", "-", "*", "/", "pow", "log", "log2", "log10", "exp", 0 };
static const char *OP_CMP[] = { "<", "<=", ">", ">=", "=", "!=", 0 };
static const char *OP_OSC[] = { "sine", "square", "sawtooth", 0 };

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
  src_string_t plain;
  src_string_t name;
  h_vec_t children;
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

typedef enum {
  MESSAGE_INFO,
  MESSAGE_WARN,
  MESSAGE_ERROR,
} message_severity_t;

static void
log_message(const ast_node_t *node, src_string_t *highlight, message_severity_t severity, const parser_ctx_t *ctx)
{
  char buf[256], *p, c, *fmt;
  size_t i, len;

  p = buf;
  *p++ = '|';
  *p++ = '-';
  *p++ = '\n';
  *p++ = '|';
  *p++ = ' ';

  for (i = 0; i < node->plain.len; i++) {
    c = *(node->plain.p + i);
    if (0 != highlight) {
      if (highlight->p == node->plain.p + i) {
        switch (severity) {
        case MESSAGE_INFO:
          fmt = COLOR_BLUE;
          len = sizeof(COLOR_BLUE);
          break;
        case MESSAGE_WARN:
          fmt = COLOR_YELLOW;
          len = sizeof(COLOR_YELLOW);
          break;
        case MESSAGE_ERROR:
          fmt = COLOR_RED;
          len = sizeof(COLOR_RED);
          break;
        }
        memcpy(p, fmt, len - 1);
        p += len - 1;
      } else if (highlight->p + highlight->len == node->plain.p + i) {
        memcpy(p, COLOR_RESET, sizeof(COLOR_RESET) - 1);
        p += sizeof(COLOR_RESET) - 1;
      }
    }
    *p++ = c;
    switch (c) {
    case '\n':
      *p++ = '|';
      *p++ = ' ';
      break;
    default:
      break;
    }
  }

  *p++ = '\n';
  *p++ = '|';
  *p++ = '-';
  *p++ = ' ';
  *p++ = '\0';

  fprintf(stderr, "%s", buf);
}

static void
parse_node(ast_node_t *node, parser_ctx_t *ctx)
{
  ast_node_t *child;

  while (is_whitespace(ctx->src[ctx->i])) {
    ctx->i++;
  }

  node->plain.p = ctx->src + ctx->i;

  ctx->i++;
  node->name.p = ctx->src + ctx->i;
  while (!is_whitespace(ctx->src[ctx->i])) {
    ctx->i++;
  }
  node->name.len = ctx->src + ctx->i - node->name.p;

  h_vec_init(&node->children, 8, sizeof(ast_node_t));

  for (;;) {
    while (is_whitespace(ctx->src[ctx->i])) {
      ctx->i++;
    }

    switch (ctx->src[ctx->i]) {
    case ')':
      ctx->i++;
      node->plain.len = ctx->src + ctx->i - node->plain.p;
      return;
    case '(':
      parse_node(h_vec_push_empty(&node->children), ctx);
      continue;
    default:
      break;
    }

    child = h_vec_push_empty(&node->children);
    child->name.p = ctx->src + ctx->i;
    child->plain.p = child->name.p;
    while (!is_whitespace(ctx->src[ctx->i]) && ')' != ctx->src[ctx->i]) {
      ctx->i++;
    }
    child->name.len = ctx->src + ctx->i - child->name.p;
    child->plain.len = child->name.len;

    memset(&child->children, 0, sizeof(h_vec_t));
  }
}

static void
free_node(ast_node_t *node)
{
  size_t i;

  if (0 == node->children.data) {
    return;
  }

  for (i = 0; i < node->children.size; i++) {
    free_node(h_vec_get(&node->children, i));
  }

  h_vec_free(&node->children);
}

static h_graph_node_t *graph_expr_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count, const parser_ctx_t *ctx);

static inline h_graph_node_t *
graph_expr_from_ast_put(h_hm_t *g, ast_node_t *an, size_t *elem_count, const parser_ctx_t *ctx)
{
  h_graph_node_t *gn;

  gn = graph_expr_from_ast(g, an, elem_count, ctx);
  h_hm_put(g, gn->name, gn);
  return gn;
}

static inline int
str_arr_includes(const char **expected, src_string_t str)
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

static h_graph_node_t *
graph_expr_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count, const parser_ctx_t *ctx)
{
  h_graph_node_t gn, *inserted, *node, **ptr;
  ast_node_t *child;
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
  } else if (-1 != (res = str_arr_includes(OP_MATH, an->name))) {
    gn.type = H_NODE_MATH;
    gn.data.math.op = res;
    h_vec_init(&gn.data.math.values, 2, sizeof(h_graph_node_t **));
    for (i = 0; i < an->children.size; i++) {
      node = graph_expr_from_ast_put(g, h_vec_get(&an->children, i), elem_count, ctx);
      h_vec_push(&gn.data.math.values, &node);
    }
  } else if (-1 != (res = str_arr_includes(OP_CMP, an->name))) {
    gn.type = H_NODE_CMP;
    gn.data.cmp.op = res;
    gn.data.cmp.left = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count, ctx);
    gn.data.cmp.right = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count, ctx);
  } else if (STR_EQ("noise", an->name)) {
    gn.type = H_NODE_NOISE;
    for (i = 0; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":seed", child->name)) {
        gn.data.noise.state = 1;
        gn.data.noise.seed = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count, ctx);
      }
    }
  } else if (-1 != (res = str_arr_includes(OP_OSC, an->name))) {
    gn.type = H_NODE_OSC;
    gn.data.osc.type = res;
    gn.data.osc.current = 0.0f;
    for (i = 0; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":freq", child->name)) {
        ptr = &gn.data.osc.freq;
      } else if (STR_EQ(":phase", child->name)) {
        ptr = &gn.data.osc.phase;
      } else {
        log_message(an, &child->plain, MESSAGE_WARN, ctx);
        fprintf(stderr, "unexpected argument for osc: '%.*s'\n", (int) child->plain.len - 1, child->plain.p + 1);
        continue;
      }
      *ptr = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count, ctx);
    }
  } else if (STR_EQ("diode", an->name)) {
    gn.type = H_NODE_DIODE;
    gn.data.diode = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count, ctx);
  } else if (STR_EQ("hardclip", an->name)) {
    gn.type = H_NODE_HARDCLIP;
    gn.data.clip.threshold = graph_expr_from_ast_put(g, h_vec_get(&an->children, 0), elem_count, ctx);
    gn.data.clip.input = graph_expr_from_ast_put(g, h_vec_get(&an->children, 1), elem_count, ctx);
  } else if (STR_EQ("bitcrush", an->name)) {
    child = h_vec_get(&an->children, 0);
    gn.type = H_NODE_BITCRUSH;
    gn.data.bitcrush.current_freq = 0.0f;
    gn.data.bitcrush.input = graph_expr_from_ast_put(g, child, elem_count, ctx);
    for (i = 1; i < an->children.size; i += 2) {
      child = h_vec_get(&an->children, i);
      if (STR_EQ(":target_freq", child->name)) {
        ptr = &gn.data.bitcrush.target_freq;
      } else if (STR_EQ(":bits", child->name)) {
        ptr = &gn.data.bitcrush.bits;
      } else {
        log_message(an, &child->plain, MESSAGE_WARN, ctx);
        fprintf(stderr, "unexpected argument for bitcrush: '%.*s'\n", (int) child->plain.len - 1, child->plain.p + 1);
        continue;
      }
      *ptr = graph_expr_from_ast_put(g, h_vec_get(&an->children, i + 1), elem_count, ctx);
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
graph_from_ast(h_hm_t *g, ast_node_t *an, size_t *elem_count, const parser_ctx_t *ctx)
{
  h_graph_node_t *gn;
  ast_node_t *child;

  if (STR_EQ("def", an->name)) {
    child = h_vec_get(&an->children, 0);
    gn = graph_expr_from_ast(g, h_vec_get(&an->children, 1), elem_count, ctx);
    memcpy(gn->name, child->name.p, child->name.len);
    gn->name[child->name.len] = '\0';
    h_hm_put(g, gn->name, gn);
  } else {
    graph_expr_from_ast_put(g, an, elem_count, ctx);
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
  for (i = 0; i < root.children.size; i++) {
    graph_from_ast(g, h_vec_get(&root.children, i), &elem_count, &ctx);
  }
  free_node(&root);
}
