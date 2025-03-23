#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <assert.h>
#include "debug.h"    // use your provided debug_printf macro
#include "malloc.h"   // contains the redefines for malloc, free, etc.

typedef struct block {
    size_t size;         // size of user data (not including header)
    struct block *next;  // next block in free list
    int free;            // 1 if free, 0 if allocated
} block_t;

#define BLOCK_SIZE sizeof(block_t)

block_t *free_list = NULL; 
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Helper: Insert block into free list sorted by address.
 */
void insert_free_block(block_t *block) {
    block->free = 1;
    if (!free_list || free_list > block) {
        block->next = free_list;
        free_list = block;
    } else {
        block_t *curr = free_list;
        while (curr->next && curr->next < block) {
            curr = curr->next;
        }
        block->next = curr->next;
        curr->next = block;
    }
}

/*
 * Helper: join adjacent free blocks.
 */
void join_free_list() {
    block_t *curr = free_list;
    while (curr && curr->next) {
        char *end_curr = (char *)(curr + 1) + curr->size;
        if (end_curr == (char *)curr->next) {
            size_t new_size = curr->size + BLOCK_SIZE + curr->next->size;
            debug_printf("free: join blocks of size %zu and %zu to new block of size %zu\n",
                         curr->size, curr->next->size, new_size);
            curr->size = new_size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}

/*
 * Malloc implementation using mmap and a free list.
 */
void *mymalloc(size_t s) {
    pthread_mutex_lock(&global_lock);
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    block_t *current = free_list, *prev = NULL;

    // Check if the request is for a small block.
    if (s <= page_size - BLOCK_SIZE) {
        // Search free list for a suitable block.
        while (current && !(current->free && current->size >= s)) {
            prev = current;
            current = current->next;
        }
        if (current) {  // Found a free block.
            current->free = 0;
            debug_printf("malloc: block of size %zu found\n", current->size);
            pthread_mutex_unlock(&global_lock);
            return (void *)(current + 1);
        }
        // No free block found; request a new page via mmap.
        debug_printf("malloc: block of size %zu not found - calling mmap\n", s);
        void *p = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            pthread_mutex_unlock(&global_lock);
            return NULL;
        }
        block_t *new_block = (block_t *)p;
        // Initially, the whole page (minus header) is available.
        new_block->size = page_size - BLOCK_SIZE;
        new_block->free = 0;
        new_block->next = NULL;
        // If the new block is larger than needed, try to split it.
        if (new_block->size >= s + BLOCK_SIZE + 1) {
            size_t remaining = new_block->size - s - BLOCK_SIZE;
            new_block->size = s;
            // Create a new free block from the remaining memory.
            block_t *split_block = (block_t *)((char *)(new_block + 1) + s);
            split_block->size = remaining;
            split_block->free = 1;
            split_block->next = free_list;
            free_list = split_block;
            debug_printf("malloc: splitting - blocks of size %zu and %zu created\n",
                         s, remaining);
        }
        pthread_mutex_unlock(&global_lock);
        return (void *)(new_block + 1);
    } 
    else {  
        // For large blocks: allocate enough pages to hold (header + data).
        size_t total_size = s + BLOCK_SIZE;
        size_t num_pages = total_size / page_size;
        if (total_size % page_size != 0)
            num_pages++;
        size_t mmap_size = num_pages * page_size;
        debug_printf("malloc: large block - mmap region of size %zu\n", mmap_size);
        void *p = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) {
            pthread_mutex_unlock(&global_lock);
            return NULL;
        }
        block_t *new_block = (block_t *)p;
        new_block->size = mmap_size - BLOCK_SIZE;
        new_block->free = 0;
        new_block->next = NULL;
        pthread_mutex_unlock(&global_lock);
        return (void *)(new_block + 1);
    }
}

/*
 * Calloc: allocate memory and set to zero.
 */
void *mycalloc(size_t nmemb, size_t s) {
    size_t total = nmemb * s;
    void *ptr = mymalloc(total);
    if (ptr) {
        // Zero out the user memory (do not overwrite the header)
        char *cptr = (char *)ptr;
        for (size_t i = 0; i < total; i++) {
            cptr[i] = 0;
        }
        debug_printf("Calloc %zu\n", total);
    }
    return ptr;
}

/*
 * Free: either return a small block to the free list or
 * munmap a large block.
 */
void myfree(void *ptr) {
    if (!ptr)
        return;
    pthread_mutex_lock(&global_lock);
    block_t *block_ptr = (block_t *)ptr - 1;
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    
    // Check if this is a small block.
    if (block_ptr->size <= page_size - BLOCK_SIZE) {
        block_ptr->free = 1;
        insert_free_block(block_ptr);
        join_free_list();
        debug_printf("Freed %zu\n", block_ptr->size);
    } else {
        // For large blocks, unmap the entire region.
        size_t total_size = block_ptr->size + BLOCK_SIZE;
        debug_printf("free: munmap region of size %zu\n", total_size);
        munmap((void *)block_ptr, total_size);
        debug_printf("Freed %zu\n", block_ptr->size);
    }
    pthread_mutex_unlock(&global_lock);
}
