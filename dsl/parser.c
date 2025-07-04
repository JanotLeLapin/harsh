#include "../harsh.h"

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
parse_node(h_dsl_node_t *node, h_dsl_parser_ctx_t *ctx)
{
  h_dsl_node_t *child;

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

  h_vec_init(&node->children, 8, sizeof(h_dsl_node_t));

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

int
h_dsl_parse(h_dsl_node_t *root, const char *src, size_t len)
{
  h_dsl_parser_ctx_t ctx = { .src = src, .src_len = len, .i = 0 };
  parse_node(root, &ctx);
  return 0;
}

void
h_dsl_free_node(h_dsl_node_t *node)
{
  size_t i;

  if (0 == node->children.data) {
    return;
  }

  for (i = 0; i < node->children.size; i++) {
    h_dsl_free_node(h_vec_get(&node->children, i));
  }

  h_vec_free(&node->children);
}
