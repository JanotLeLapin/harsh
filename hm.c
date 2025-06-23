#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "harsh.h"

static inline int
resize(h_hm_t *hm)
{
  h_hm_entry_t **new_buckets, *entry, *next;
  size_t new_capacity, i;
  uint64_t hash;

  new_capacity = hm->capacity * (1 + hm->load_factor);
  new_buckets = calloc(new_capacity, sizeof(h_hm_entry_t *));
  if (0 == new_buckets) {
    return -1;
  }

  for (i = 0; i < hm->capacity; i++) {
    entry = hm->buckets[i];
    while (entry) {
      next = entry->next;
      hash = hm->hash_func(entry->key) % new_capacity;
      entry->next = new_buckets[hash];
      new_buckets[hash] = entry;
      entry = next;
    }
  }

  free(hm->buckets);
  hm->buckets = new_buckets;
  hm->capacity = new_capacity;

  return 0;
}

int
h_hm_init(h_hm_t *hm, size_t initial_capacity, float load_factor, h_hm_hash_func_t hash_func, h_hm_eq_func_t eq_func)
{
  hm->buckets = calloc(initial_capacity, sizeof(struct hashmap_entry *));
  if (0 == hm->buckets) {
    return -1;
  }
  hm->size = 0;
  hm->capacity = initial_capacity;
  hm->load_factor = load_factor;
  hm->hash_func = hash_func;
  hm->eq_func = eq_func;

  return 0;
}

int
h_hm_put(h_hm_t *hm, const void *key, void *value)
{
  size_t idx;
  h_hm_entry_t *entry, *new_entry;

  idx = hm->hash_func(key) % hm->capacity;
  entry = hm->buckets[idx];

  while (entry) {
    if (hm->eq_func(entry->key, key)) {
      entry->value = value;
      return 0;
    }
    entry = entry->next;
  }

  new_entry = malloc(sizeof(h_hm_entry_t));
  if (0 == new_entry) {
    return -1;
  }
  new_entry->key = key;
  new_entry->value = value;
  new_entry->next = hm->buckets[idx];

  hm->buckets[idx] = new_entry;
  hm->size++;

  return 0;
}

void *
h_hm_get(h_hm_t *hm, const void *key)
{
  size_t idx;
  h_hm_entry_t *entry;
  void *res = 0;

  idx = hm->hash_func(key) % hm->capacity;
  entry = hm->buckets[idx];

  while (entry) {
    if (hm->eq_func(key, entry->key)) {
      res = entry->value;
      break;
    }
    entry = entry->next;
  }

  return res;
}

void
h_hm_remove(h_hm_t *hm, const void *key)
{
  size_t idx;
  h_hm_entry_t *entry, *prev = 0;

  idx = hm->hash_func(key) % hm->capacity;
  entry = hm->buckets[idx];

  while (entry) {
    if (hm->eq_func(key, entry->key)) {
      if (prev) {
        prev->next = entry->next;
      } else {
        hm->buckets[idx] = entry->next;
      }
      free(entry);
      hm->size--;
      break;
    }
    prev = entry;
    entry = entry->next;
  }
}

void
h_hm_free(h_hm_t *hm)
{
  size_t i;
  h_hm_entry_t *entry, *prev;

  for (i = 0; i < hm->capacity; i++) {
    entry = hm->buckets[i];
    while (entry) {
      prev = entry;
      entry = entry->next;
      free(prev);
    }
  }

  free(hm->buckets);
}
