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

#endif
