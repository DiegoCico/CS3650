#ifndef INODE_H
#define INODE_H

#include "blocks.h"

#define NUM_INODES 128
#define MAX_BLOCKS_PER_FILE 128

typedef struct inode {
  int refs;   // reference count
  int mode;   // permission & type
  int size;   // file size in bytes
  int block;  // primary block (if file fits within one block) or first block pointer
} inode_t;

void print_inode(inode_t *node);
inode_t *get_inode(int inum);
int alloc_inode();
int free_inode(int inum);
int grow_inode(inode_t *node, int size);
int shrink_inode(inode_t *node, int size);
int inode_get_bnum(inode_t *node, int file_bnum);

#endif
