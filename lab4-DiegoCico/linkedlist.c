#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "linkedlist.h"

node_t *cons(int data, node_t *list) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (!new_node) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }
    new_node->data = data;
    new_node->next = list;
    return new_node;
}

int first(node_t *list) {
    if (list == NULL) {
        fprintf(stderr, "Error: first() called on an empty list\n");
        exit(EXIT_FAILURE);
    }
    return list->data;
}

node_t *rest(node_t *list) {
    if (list == NULL) {
        return NULL;
    }
    return list->next;
}

int is_empty(node_t *list) {
    return list == NULL;
}

void print_list(node_t *list) {
    node_t *current = list;
    while (current != NULL) {
        printf("%d\n", current->data);
        current = current->next;
    }
}

void free_list(node_t *list) {
    node_t *current = list;
    while (current != NULL) {
        node_t *next = current->next;
        free(current);
        current = next;
    }
}

