/*
 * Queue implementation.
 *
 * - Implement each of the functions to create a working circular queue.
 * - Do not change any of the structs
 * - When submitting, You should not have any 'printf' statements in your queue 
 *   functions. 
 */
#include <assert.h>
#include <stdlib.h>

#include "queue.h"

/** The main data structure for the queue. */
struct queue{
  unsigned int back;      /* The next free position in the queue (i.e. tail) */
  unsigned int front;     /* Current head of the queue (i.e. front) */
  unsigned int size;      /* How many total elements are enqueued. */
  unsigned int capacity;  /* Maximum number of items the queue can hold */
  long *data;             /* The data our queue holds */
};

/**
 * Construct a new empty queue.
 *
 * Returns a pointer to a newly created queue.
 * Return NULL on error
 */
queue_t *queue_new(unsigned int capacity) {
  queue_t *q = malloc(sizeof(queue_t));
  if (q == NULL) {
    return NULL;
  }

  q->data = malloc(capacity * sizeof(long));
  if (q->data == NULL) {
    free(q);
    return NULL;
  }

  q->capacity = capacity;
  q->size = 0;
  q->front = 0;
  q->back = 0;

  return q;
}

/**
 * Check if the given queue is empty.
 *
 * Returns a non-0 value if the queue is empty, 0 otherwise.
 */
int queue_empty(queue_t *q) {
  assert(q != NULL);
  return (q->size == 0);
}

/**
 * Check if the given queue is full.
 *
 * Returns a non-0 value if the queue is full, 0 otherwise.
 */
int queue_full(queue_t *q) {
  assert(q != NULL);
  return (q->size == q->capacity);
}

/**
 * Enqueue a new item.
 *
 * Push a new item into our data structure.
 */
void queue_enqueue(queue_t *q, long item) {
  assert(q != NULL);
  assert(q->size < q->capacity);  

  q->data[q->back] = item;
  
 
  q->back = (q->back + 1) % q->capacity;
  

  q->size++;
}

/**
 * Dequeue an item.
 *
 * Returns the item at the front of the queue and removes an item from the 
 * queue.
 *
 * Note: Removing from an empty queue is undefined (i.e., it could crash the program)
 */
long queue_dequeue(queue_t *q) {
  assert(q != NULL);
  assert(q->size > 0);  

  
  long item = q->data[q->front];
  q->front = (q->front + 1) % q->capacity;
  q->size--;

  return item;
}

/**
 * Queue size.
 *
 * Queries the current size of a queue (valid size must be >= 0).
 */
unsigned int queue_size(queue_t *q) {
  assert(q != NULL);
  return q->size;
}

/**
 * Delete queue.
 * 
 * Remove the queue and all of its elements from memory.
 *
 * Note: This should be called before the program terminates.
 */
void queue_delete(queue_t* q) {
  assert(q != NULL);
  free(q->data);
  free(q);
}

