// helper code
#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdio.h>

extern const int BLOCK_COUNT;   // Number of blocks (default 256)
extern const int BLOCK_SIZE;    // Block size (4096 bytes)
extern const int NUFS_SIZE;     // Disk image size (1MB)
extern const int BLOCK_BITMAP_SIZE; // Size of block bitmap in bytes

int bytes_to_blocks(int bytes);
void blocks_init(const char *image_path);
void blocks_free();
void *blocks_get_block(int bnum);
void *get_blocks_bitmap();
void *get_inode_bitmap();
int alloc_block();
void free_block(int bnum);

#endif
