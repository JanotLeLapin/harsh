#include "harsh.h"

#define EXPORT_SIZE(type_name) \
  size_t \
  w_##type_name##_size() \
  { \
    return sizeof(type_name); \
  }

EXPORT_SIZE(h_hm_t)
EXPORT_SIZE(h_context)

EXPORT_SIZE(h_dsl_string_t)
EXPORT_SIZE(h_dsl_node_t)
EXPORT_SIZE(h_dsl_error_t)
EXPORT_SIZE(h_dsl_ctx_t)

size_t
w_block_size()
{
  return BLOCK_SIZE;
}

void
w_context_init(h_context *ctx, float sr)
{
  ctx->current_block = 0;
  ctx->sr = sr;
}

void
w_dsl_ctx_init(h_dsl_ctx_t *ctx, char *src, size_t len)
{
  ctx->src = src;
  ctx->src_len = len;
  ctx->error_count = 0;
  ctx->failed = 0;
}

void
w_graph_render_block(h_hm_t *g, const char *out, h_context *ctx, float *buf)
{
  h_graph_node_t *node;
  size_t i;

  node = h_hm_get(g, out);

  h_graph_process_node(g, node, ctx);
  for (i = 0; i < BLOCK_SIZE; i++) {
    buf[i * 2] = node->out[0][i];
    buf[i * 2 + 1] = node->out[1][i];
  }

  ctx->current_block++;
}
