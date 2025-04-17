 // helper code
 #define _GNU_SOURCE
 #include <string.h>
 #include <assert.h>
 #include <errno.h>
 #include <fcntl.h>
 #include <stdint.h>
 #include <stdio.h>
 #include <sys/mman.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <unistd.h>
 #include <stdlib.h>
 
 #include "bitmap.h"
 #include "blocks.h"
 
 const int BLOCK_COUNT = 256;          // 256 blocks in the disk image
 const int BLOCK_SIZE = 4096;          // 4K per block
 const int NUFS_SIZE = BLOCK_SIZE * BLOCK_COUNT; // Total size: 1MB
 
 const int BLOCK_BITMAP_SIZE = BLOCK_COUNT / 8;  // Bitmap size in bytes
 
 static int blocks_fd = -1;
 static void *blocks_base = 0;
 
 int bytes_to_blocks(int bytes) {
   int quo = bytes / BLOCK_SIZE;
   int rem = bytes % BLOCK_SIZE;
   return (rem == 0) ? quo : quo + 1;
 }
 
 void blocks_init(const char *image_path) {
    blocks_fd = open(image_path, O_CREAT | O_RDWR, 0644);
    assert(blocks_fd != -1);
    int rv = ftruncate(blocks_fd, NUFS_SIZE);
    assert(rv == 0);
    blocks_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
    assert(blocks_base != MAP_FAILED);
    memset(blocks_base, 0, NUFS_SIZE);
    void *bbm = get_blocks_bitmap();
    bitmap_put(bbm, 0, 1);
  }
  
 
 void blocks_free() {
   int rv = munmap(blocks_base, NUFS_SIZE);
   assert(rv == 0);
 }
 
 void *blocks_get_block(int bnum) {
   return blocks_base + BLOCK_SIZE * bnum;
 }
 
 void *get_blocks_bitmap() {
   return blocks_get_block(0);
 }
 
 void *get_inode_bitmap() {
   uint8_t *block = blocks_get_block(0);
   return (void *) (block + BLOCK_BITMAP_SIZE);
 }
 
 int alloc_block() {
   void *bbm = get_blocks_bitmap();
   for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
     if (!bitmap_get(bbm, ii)) {
       bitmap_put(bbm, ii, 1);
       printf("+ alloc_block() -> %d\n", ii);
       return ii;
     }
   }
   return -1;
 }
 
 void free_block(int bnum) {
   printf("+ free_block(%d)\n", bnum);
   void *bbm = get_blocks_bitmap();
   bitmap_put(bbm, bnum, 0);
 }
 