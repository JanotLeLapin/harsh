#include "harsh.h"

#define EXPORT_SIZE(type_name) \
  size_t \
  w_##type_name##_size() \
  { \
    return sizeof(type_name); \
  }

EXPORT_SIZE(h_hm_t)
EXPORT_SIZE(h_context)

void
w_context_init(h_context *ctx, float sr)
{
  ctx->current_frame = 0;
  ctx->sr = sr;
}

void
w_graph_render_block(h_hm_t *g, const char *out, h_context *ctx, float *buf, size_t buf_size)
{
  h_graph_render_block(g, h_hm_get(g, out), ctx, buf, buf_size);
}
