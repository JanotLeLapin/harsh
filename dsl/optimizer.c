#include <stdio.h>

#include "../dsl.h"

#define CHECK(n) (H_NODE_VALUE == n->type)

static inline int
is_optimizable(h_graph_node_t *node)
{
  size_t i;
  h_graph_node_t *child;

  switch (node->type) {
  case H_NODE_VALUE:
  case H_NODE_NOISE:
  case H_NODE_OSC:
  case H_NODE_ENVELOPE:
  #ifndef __EMSCRIPTEN__
  case H_NODE_AUDIO:
  #endif
    return 0;
  case H_NODE_MATH:
    for (i = 0; i < node->data.math.values.size; i++) {
      child = *(h_graph_node_t **) h_vec_get(&node->data.math.values, i);
      if (!CHECK(child)) {
        return 0;
      }
    }
    break;
  case H_NODE_CMP:
    if (!CHECK(node->data.cmp.left) || !CHECK(node->data.cmp.right)) {
      return 0;
    }
    break;
  case H_NODE_CONVERSION:
    if (!CHECK(node->data.conversion.input)) {
      return 0;
    }
    break;
  case H_NODE_DIODE:
    if (!CHECK(node->data.diode)) {
      return 0;
    }
    break;
  case H_NODE_CLIP:
    if (!CHECK(node->data.clip.input)) {
      return 0;
    }
    break;
  case H_NODE_FILTER:
    if (!CHECK(node->data.filter.input)) {
      return 0;
    }
    break;
  case H_NODE_BITCRUSH:
    if (!CHECK(node->data.bitcrush.input)) {
      return 0;
    }
    break;
  }

  return 1;
}

int
h_dsl_optimize(h_hm_t *g)
{
  char optimized;
  size_t i;
  h_hm_entry_t *entry;
  h_graph_node_t *node, *new;
  h_context ctx = { .current_frame = 1, .sr = 44100.0 };

  do {
    optimized = 0;
    for (i = 0; i < g->capacity; i++) {
      entry = g->buckets[i];
      while (entry) {
        node = entry->value;
        if (is_optimizable(node)) {
          optimized = 1;
          h_graph_process_node(g, node, &ctx);
          if (H_NODE_MATH == node->type) {
            h_vec_free(&node->data.math.values);
          }
          node->type = H_NODE_VALUE;
          node->last_frame = 0;
          memset(&node->data, 0, sizeof(h_graph_node_data_t));
        }
        entry = entry->next;
      }
    }
  } while (optimized);

  return 0;
}
