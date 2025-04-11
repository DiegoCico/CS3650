#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "bitmap.h"
#include "blocks.h"

#define NUM_INODES 128
#define MAX_BLOCKS_PER_FILE 128
#define BLOCK_SIZE 4096

/* Global inode table and allocation flags */
inode_t inode_table[NUM_INODES];
static int inode_allocated[NUM_INODES] = {0};

/* Global per-inode block lists for multi-block files */
static int inode_blocks[NUM_INODES][MAX_BLOCKS_PER_FILE];

void print_inode(inode_t *node) {
    if (!node) return;
    int inum = node - inode_table;
    printf("inode %d: mode=%o, size=%d, block=%d\n", inum, node->mode, node->size, node->block);
}

inode_t *get_inode(int inum) {
    if (inum < 0 || inum >= NUM_INODES) return NULL;
    if (!inode_allocated[inum]) return NULL;
    return &inode_table[inum];
}

int alloc_inode() {
    for (int i = 0; i < NUM_INODES; i++) {
        if (!inode_allocated[i]) {
            inode_allocated[i] = 1;
            inode_table[i].refs = 1;
            inode_table[i].mode = 0;
            inode_table[i].size = 0;
            inode_table[i].block = -1;
            for (int j = 0; j < MAX_BLOCKS_PER_FILE; j++)
                inode_blocks[i][j] = -1;
            return i;
        }
    }
    return -1; // ENOSPC
}

int free_inode(int inum) {
    if (inum < 0 || inum >= NUM_INODES)
        return -1;
    inode_allocated[inum] = 0;
    memset(&inode_table[inum], 0, sizeof(inode_t));
    return 0;
}

int grow_inode(inode_t *node, int size) {
    if (!node) return -1;
    int inum = node - inode_table;
    int old_blocks = (node->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int new_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int b = old_blocks; b < new_blocks; b++) {
        int blk = alloc_block();
        if (blk < 0)
            return -1;
        if (b == 0) {
            node->block = blk;
        }
        inode_blocks[inum][b] = blk;
    }
    node->size = size;
    return 0;
}

int shrink_inode(inode_t *node, int size) {
    if (!node) return -1;
    int inum = node - inode_table;
    int old_blocks = (node->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int new_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int b = new_blocks; b < old_blocks; b++) {
        int blk = inode_blocks[inum][b];
        if (blk >= 0) {
            free_block(blk);
            inode_blocks[inum][b] = -1;
        }
    }
    node->size = size;
    return 0;
}

int inode_get_bnum(inode_t *node, int file_bnum) {
    if (!node) return -1;
    int inum = node - inode_table;
    int nblocks = (node->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (file_bnum < 0 || file_bnum >= nblocks)
        return -1;
    if (file_bnum == 0)
        return inode_blocks[inum][0];
    return inode_blocks[inum][file_bnum];
}
