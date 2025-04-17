// helper code
#ifndef SLIST_H
#define SLIST_H

typedef struct slist {
  void *data;
  int refs;
  struct slist *next;
} slist_t;

slist_t *slist_cons(const char *text, slist_t *rest);
slist_t *slist_cons_ptr(void *ptr, slist_t *rest);
void slist_free(slist_t *xs);
slist_t *slist_explode(const char *text, char delim);

#endif
