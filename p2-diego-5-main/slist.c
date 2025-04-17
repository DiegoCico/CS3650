// helper code
#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include "slist.h"

slist_t *slist_cons(const char *text, slist_t *rest) {
  slist_t *xs = malloc(sizeof(slist_t));
  xs->data = strdup(text);
  xs->refs = 1;
  xs->next = rest;
  return xs;
}

slist_t *slist_cons_ptr(void *ptr, slist_t *rest) {
  slist_t *xs = malloc(sizeof(slist_t));
  xs->data = ptr;
  xs->refs = 1;
  xs->next = rest;
  return xs;
}

void slist_free(slist_t *xs) {
  if (xs == 0)
    return;
  xs->refs -= 1;
  if (xs->refs == 0) {
    slist_free(xs->next);
    free(xs->data);
    free(xs);
  }
}

slist_t *slist_explode(const char *text, char delim) {
  if (*text == 0)
    return 0;
  int plen = 0;
  while (text[plen] != 0 && text[plen] != delim)
    plen += 1;
  int skip = (text[plen] == delim) ? 1 : 0;
  slist_t *rest = slist_explode(text + plen + skip, delim);
  char *part = alloca(plen + 2);
  memcpy(part, text, plen);
  part[plen] = 0;
  return slist_cons(part, rest);
}
