#ifndef _H_HARSH_DSL
#define _H_HARSH_DSL

#include <stdlib.h>

#include "harsh.h"

typedef struct {
  const char *src;
  size_t src_len;
  size_t i;
} h_dsl_parser_ctx_t;

typedef struct {
  const char *p;
  size_t len;
} h_dsl_string_t;

typedef struct {
  h_dsl_string_t plain;
  h_dsl_string_t name;
  h_vec_t children;
} h_dsl_node_t;

int h_dsl_parse(h_dsl_node_t *root, const char *src, size_t len);
int h_dsl_compile(h_hm_t *g, h_dsl_node_t *root);

void h_dsl_free_node(h_dsl_node_t *node);

#endif
