#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

typedef struct node {
    int data;
    struct node* next;
} node_t;

/** Prepend an item to the front of the given list */
node_t *cons(int data, node_t *list);

/** Get the first item in the list */
int first(node_t *list);

/** Get the rest of the list. */
node_t *rest(node_t *list);

/** Is the list empty? Return 0 if it is not empty, non-0 otherwise */
int is_empty(node_t *list);

/** Print the whole list, one item at a time. */
void print_list(node_t *list);

/** Free the memory held by the whole list. */
void free_list(node_t *list);



#endif /* _LINKEDLIST_H */
