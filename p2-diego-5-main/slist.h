/**
 * @file slist.h
 * A simple linked list of pointers.
 */
#ifndef SLIST_H
#define SLIST_H

typedef struct slist {
  void *data;
  int refs;
  struct slist *next;
} slist_t;

/* For string lists, this copies the given string. */
slist_t *slist_cons(const char *text, slist_t *rest);
/* For generic pointer lists, this simply stores the pointer. */
slist_t *slist_cons_ptr(void *ptr, slist_t *rest);
void slist_free(slist_t *xs);
slist_t *slist_explode(const char *text, char delim);

#endif
