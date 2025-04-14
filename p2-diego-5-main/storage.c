#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "storage.h"
#include "blocks.h"

// set up storage system with the disk image path
void storage_init(const char *path) {
    blocks_init(path); 
    printf("Storage set up. Disk image: %s\n", path);
}

// get file stats
int storage_stat(const char *path, struct stat *st) {
    if (!st) return -1; 
    st->st_size = BLOCK_SIZE * 256; 
    return 0;
}

// read from block 0 starting at offset into buf
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    memcpy(buf, blocks_get_block(0) + offset, size); // copy data into buf
    return size; 
}

// write to block 0 starting at offset from buf
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    memcpy(blocks_get_block(0) + offset, buf, size); // copy data from buf
    return size;
}

// read a piece of a specific block into buf
int storage_read_block(int bnum, char *buf, int offset, int size) {
    void *block = blocks_get_block(bnum); 
    memcpy(buf, block + offset, size); // copy the data
    return size;
}

// write a piece of data into a specific block
int storage_write_block(int bnum, const char *buf, int offset, int size) {
    void *block = blocks_get_block(bnum); 
    memcpy(block + offset, buf, size); // write the data
    return size; 
}
