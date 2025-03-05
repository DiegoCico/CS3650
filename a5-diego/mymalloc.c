#define _DEFAULT_SOURCE
#define _BSD_SOURCE 
#include <unistd.h>
#include <stdio.h>
#include <debug.h>

typedef struct block {
    size_t size;        
    struct block *next; 
    int free;          
} block_t;

#define BLOCK_SIZE sizeof(block_t)

block_t *free_list = NULL; 

/*
 * mymalloc -  memory of size `s`
 * Looks for free space, if not found, gets more memory.
 */
void *mymalloc(size_t s) {
    block_t *current = free_list, *prev = NULL;
    while (current && !(current->free && current->size >= s)) {
        prev = current;
        current = current->next;
    }
    if (current) { // a free block
        current->free = 0;
        debug_printf("Malloc %zu\n", s);
        return (void *)(current + 1);
    }
    // No free block, get more memory
    void *p = sbrk(s + BLOCK_SIZE);
    if (p == (void *)-1) return NULL;
    block_t *new_block = (block_t *)p;
    new_block->size = s;
    new_block->free = 0;
    new_block->next = NULL;
    if (prev) prev->next = new_block;
    else free_list = new_block;
    debug_printf("Malloc %zu\n", s);
    return (void *)(new_block + 1);
}

/*
 * mycalloc - `mem * s` bytes, sets to zero
 */
void *mycalloc(size_t mem, size_t s) {
    size_t total_size = mem * s;
    void *p = mymalloc(total_size);
    if (p) {
        char *ptr = (char *)p;
        for (size_t i = 0; i < total_size; i++) ptr[i] = 0;
        debug_printf("Calloc %zu\n", total_size);
    }
    return p;
}

/*
 * myfree - Frees memory block
 * Marks block as free so it can be reused.
 */
void myfree(void *ptr) {
    if (!ptr) return;
    block_t *block_ptr = (block_t *)ptr - 1;
    block_ptr->free = 1;
    debug_printf("Freed %zu\n", block_ptr->size);
}
