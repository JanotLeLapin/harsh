#include <stdlib.h>
#include <string.h>

#include "harsh.h"

int
h_vec_init(h_vec_t *v, size_t initial_capacity, size_t elem_size)
{
  v->capacity = initial_capacity;
  v->size = 0;
  v->elem_size = elem_size;
  v->data = malloc(v->capacity * v->elem_size);

  return 0 == v->data ? -1 : 0;
}

void *
h_vec_push(h_vec_t *v, void *data)
{
  size_t new_capacity;
  char *new_data, *ptr;

  if (v->capacity <= v->size) {
    new_capacity = v->capacity * 2;
    new_data = realloc(v->data, new_capacity * v->elem_size);
    if (0 == new_data) {
      return 0;
    }
    v->capacity = new_capacity;
    v->data = new_data;
  }

  ptr = v->data + v->size * v->elem_size;
  memcpy(ptr, data, v->elem_size);
  v->size++;
  return ptr;
}

void *
h_vec_push_empty(h_vec_t *v)
{
  size_t new_capacity;
  char *new_data, *ptr;

  if (v->capacity <= v->size) {
    new_capacity = v->capacity * 2;
    new_data = realloc(v->data, new_capacity * v->elem_size);
    if (0 == new_data) {
      return 0;
    }
    v->capacity = new_capacity;
    v->data = new_data;
  }

  ptr = v->data + v->size * v->elem_size;
  v->size++;
  return ptr;
}

void *
h_vec_get(h_vec_t *v, int index)
{
  return v->data + index * v->elem_size;
}

void
h_vec_remove(h_vec_t *v, int index)
{
  v->size--;
  memcpy(v->data + index * v->elem_size, v->data + v->size * v->elem_size, v->elem_size);
}

void
h_vec_free(h_vec_t *v)
{
  free(v->data);
  memset(v, 0, sizeof(h_vec_t));
}
