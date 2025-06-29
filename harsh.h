#ifndef _H_HARSH
#define _H_HARSH

#include <stdlib.h>
#include <string.h>

static const char *H_OP_MATH[] = { "+", "-", "*", "/", "pow", "log", "log2", "log10", "exp", 0 };
static const char *H_OP_CMP[] = { "<", "<=", ">", ">=", "=", "!=", 0 };
static const char *H_OP_CONVERSION[] = { "midi->freq", "freq->midi", "db->amp", "amp->db", 0 };
static const char *H_OP_OSC[] = { "sine", "square", "sawtooth", 0 };
static const char *H_OP_CLIP[] = { "hardclip", "foldback", 0 };
static const char *H_OP_FILTER[] = { "lowpass", "highpass", 0 };

/* util */
typedef struct {
  size_t capacity;
  size_t size;
  size_t elem_size;
  void *data;
} h_vec_t;

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
typedef struct h_graph_node_s h_graph_node_t;

typedef void *h_node_value_t;

typedef struct {
  enum {
    H_NODE_MATH_ADD = 0,
    H_NODE_MATH_SUB,
    H_NODE_MATH_MUL,
    H_NODE_MATH_DIV,
    H_NODE_MATH_POW,
    H_NODE_MATH_LOGN,
    H_NODE_MATH_LOG2,
    H_NODE_MATH_LOG10,
    H_NODE_MATH_EXP,
  } op;
  h_vec_t values;
} h_node_math_t;

typedef struct {
  enum {
    H_NODE_CMP_LT = 0,
    H_NODE_CMP_LEQT,
    H_NODE_CMP_GT,
    H_NODE_CMP_GEQT,
    H_NODE_CMP_EQ,
    H_NODE_CMP_NEQ,
  } op;
  h_graph_node_t *left;
  h_graph_node_t *right;
} h_node_cmp_t;

typedef struct {
  enum {
    H_NODE_CONVERSION_MTOF = 0,
    H_NODE_CONVERSION_FTOM,
    H_NODE_CONVERSION_DTOA,
    H_NODE_CONVERSION_ATOD,
  } op;
  h_graph_node_t *input;
} h_node_conversion_t;

typedef struct {
  unsigned int state;
  h_graph_node_t *seed;
} h_node_noise_t;

typedef struct {
  enum {
    H_NODE_OSC_SINE = 0,
    H_NODE_OSC_SQUARE,
    H_NODE_OSC_SAWTOOTH,
  } type;
  float current;
  h_graph_node_t *freq;
  h_graph_node_t *phase;
} h_node_osc_t;

typedef h_graph_node_t *h_node_diode_t;

typedef struct {
  enum {
    H_NODE_CLIP_HARDCLIP = 0,
    H_NODE_CLIP_FOLDBACK,
  } type;
  h_graph_node_t *threshold;
  h_graph_node_t *input;
} h_node_clip_t;

typedef struct {
  enum {
    H_NODE_FILTER_LOWPASS = 0,
    H_NODE_FILTER_HIGHPASS,
  } type;
  float in[4];
  float out[4];
  h_graph_node_t *cutoff;
  h_graph_node_t *stages;
  h_graph_node_t *input;
} h_node_filter_t;

typedef struct {
  float current_freq;
  h_graph_node_t *input;
  h_graph_node_t *target_freq;
  h_graph_node_t *bits;
} h_node_bitcrush_t;

typedef struct {
  size_t current_idx;
  h_vec_t points;
} h_node_envelope_t;

typedef enum {
  H_NODE_VALUE,
  H_NODE_MATH,
  H_NODE_CMP,

  H_NODE_CONVERSION,

  H_NODE_NOISE,
  H_NODE_OSC,

  H_NODE_DIODE,
  H_NODE_CLIP,

  H_NODE_FILTER,

  H_NODE_BITCRUSH,

  H_NODE_ENVELOPE,
} h_graph_node_type_t;

typedef union {
  h_node_value_t value;
  h_node_math_t math;
  h_node_cmp_t cmp;

  h_node_conversion_t conversion;

  h_node_noise_t noise;
  h_node_osc_t osc;

  h_node_diode_t diode;
  h_node_clip_t clip;

  h_node_filter_t filter;

  h_node_bitcrush_t bitcrush;

  h_node_envelope_t envelope;
} h_graph_node_data_t;

struct h_graph_node_s {
  char name[32];
  float out;
  size_t last_frame;
  h_graph_node_type_t type;
  h_graph_node_data_t data;
};

/* util */
int h_vec_init(h_vec_t *v, size_t initial_capacity, size_t elem_size);
void *h_vec_push(h_vec_t *v, void *data);
void *h_vec_push_empty(h_vec_t *v);
void *h_vec_get(h_vec_t *v, int index);
void h_vec_remove(h_vec_t *v, int index);
void h_vec_free(h_vec_t *v);

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
void h_graph_render_block(h_hm_t *g, h_graph_node_t *out, h_context *ctx, float *buf, size_t buf_size);
int h_graph_render_wav32(const char *filename, h_hm_t *g, h_context *ctx, size_t sample_count, size_t buf_size);
void h_graph_free(h_hm_t *g);

/* dsl */
void h_dsl_load(h_hm_t *g, const char *src, size_t src_len);

#endif
