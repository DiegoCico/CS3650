/**
 * Vector implementation.
 *
 * - Implement each of the functions to create a working growable array (vector).
 * - Do not change any of the structs
 * - When submitting, You should not have any 'printf' statements in your vector 
 *   functions.
 *
 * IMPORTANT: The initial capacity and the vector's growth factor should be 
 * expressed in terms of the configuration constants in vect.h
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vect.h"

/** Main data structure for the vector. */
struct vect {
  char **data;             /* Array containing the actual data. */
  unsigned int size;       /* Number of items currently in the vector. */
  unsigned int capacity;   /* Maximum number of items the vector can hold before growing. */
};

/* Custom implementation of strdup for ISO C11 */
static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *copy = malloc(len);
    if (copy != NULL) {
        memcpy(copy, s, len);
    }
    return copy;
}

/** Construct a new empty vector. */
vect_t *vect_new() {
  vect_t *v = malloc(sizeof(vect_t));
  if (v == NULL) {
    return NULL;
  }
  v->capacity = VECT_INITIAL_CAPACITY;
  v->size = 0;
  v->data = malloc(v->capacity * sizeof(char *));
  if (v->data == NULL) {
    free(v);
    return NULL;
  }
  return v;
}

/** Delete the vector, freeing all memory it occupies. */
void vect_delete(vect_t *v) {
  if (v == NULL) {
    return;
  }
  for (unsigned int i = 0; i < v->size; i++) {
    free(v->data[i]);
  }
  free(v->data);
  free(v);
}

/** Get the element at the given index. */
const char *vect_get(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);
  return v->data[idx];
}

/** 
 * Get a copy of the element at the given index. 
 * The caller is responsible for freeing the memory occupied by the copy.
 */
char *vect_get_copy(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);
  char *copy = my_strdup(v->data[idx]);
  assert(copy != NULL);
  return copy;
}

/** Set the element at the given index. */
void vect_set(vect_t *v, unsigned int idx, const char *elt) {
  assert(v != NULL);
  assert(idx < v->size);
  assert(elt != NULL);
  free(v->data[idx]);
  char *copy = my_strdup(elt);
  assert(copy != NULL);
  v->data[idx] = copy;
}

/** Add an element to the back of the vector. */
void vect_add(vect_t *v, const char *elt) {
  assert(v != NULL);
  assert(elt != NULL);

  if (v->size == v->capacity) {
    unsigned int new_capacity = v->capacity * VECT_GROWTH_FACTOR;
    if (new_capacity > VECT_MAX_CAPACITY) {
      new_capacity = VECT_MAX_CAPACITY;
    }
    char **new_data = realloc(v->data, new_capacity * sizeof(char *));
    assert(new_data != NULL);
    v->data = new_data;
    v->capacity = new_capacity;
  }
  
  char *copy = my_strdup(elt);
  assert(copy != NULL);
  v->data[v->size] = copy;
  v->size++;
}

/** Remove the last element from the vector. */
void vect_remove_last(vect_t *v) {
  assert(v != NULL);
  if (v->size == 0) {
    return; 
  }

  free(v->data[v->size - 1]);
  v->size--;
}

/** The number of items currently in the vector. */
unsigned int vect_size(vect_t *v) {
  assert(v != NULL);
  return v->size;
}

/** The maximum number of items the vector can hold before it has to grow. */
unsigned int vect_current_capacity(vect_t *v) {
  assert(v != NULL);
  return v->capacity;
}

