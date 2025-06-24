#ifndef _H_HARSH
#define _H_HARSH

#include <stdlib.h>
#include <string.h>

/* util */
typedef char (*h_hm_eq_func_t)(const void *a, const void *b);
typedef size_t (*h_hm_hash_func_t)(const void *key);

typedef struct h_hm_entry_s {
  const void *key;
  void *value;
  struct h_hm_entry_s *next;
} h_hm_entry_t;

typedef struct {
  size_t capacity;
  size_t size;
  float load_factor;
  h_hm_hash_func_t hash_func;
  h_hm_eq_func_t eq_func;
  h_hm_entry_t **buckets;
} h_hm_t;

/* core */
typedef struct {
  size_t current_frame;
  float sr;
} h_context;

/* dsp */
typedef void *h_node_value_t;

typedef struct {
  enum {
    H_NODE_MATH_ADD,
    H_NODE_MATH_MUL,
  } op;
  char *left;
  char *right;
} h_node_math_t;

typedef struct {
  unsigned int state;
  char *seed;
} h_node_noise_t;

typedef struct {
  enum {
    H_NODE_OSC_SINE,
    H_NODE_OSC_SQUARE,
    H_NODE_OSC_SAWTOOTH,
  } type;
  float current;
  char *freq;
  char *phase;
} h_node_osc_t;

typedef enum {
  H_NODE_VALUE,
  H_NODE_MATH,

  H_NODE_NOISE,
  H_NODE_OSC,
} h_graph_node_type_t;

typedef union {
  h_node_value_t value;
  h_node_math_t math;

  h_node_noise_t noise;
  h_node_osc_t osc;
} h_graph_node_data_t;

typedef struct {
  char name[32];
  float out;
  size_t last_frame;
  h_graph_node_type_t type;
  h_graph_node_data_t data;
} h_graph_node_t;

/* util */
static size_t
h_hash_string(const void *key)
{
  int c;
  const char *str = (const char *) key;
  size_t hash = 5381;

  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return hash;
}

static char
h_eq_string(const void *a, const void *b)
{
  return 0 == strcmp((char *) a, (char *) b);
}

int h_hm_init(h_hm_t *hm, size_t initial_capacity, float load_factor, h_hm_hash_func_t hash_func, h_hm_eq_func_t eq_func);
int h_hm_put(h_hm_t *hm, const void *key, void *value);
void *h_hm_get(h_hm_t *hm, const void *key);
void h_hm_remove(h_hm_t *hm, const void *key);
void h_hm_free(h_hm_t *hm);

/* dsp */
void h_graph_process_node(h_hm_t *g, h_graph_node_t *node, const h_context *ctx);
void h_graph_preview(h_hm_t *g);
int h_graph_render_wav32(const char *filename, h_hm_t *g, h_context *ctx, size_t sample_count, size_t buf_size);
void h_graph_free(h_hm_t *g);

/* dsl */
void h_dsl_load(h_hm_t *g, const char *src, size_t src_len);

#endif
