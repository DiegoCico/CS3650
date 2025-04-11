#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "storage.h"
#include "blocks.h"

void storage_init(const char *path) {
    blocks_init(path);
    printf("Storage initialized. Disk image: %s\n", path);
}

int storage_stat(const char *path, struct stat *st) {
    if (!st) return -1;
    st->st_size = BLOCK_SIZE * 256;
    return 0;
}

int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    memcpy(buf, blocks_get_block(0) + offset, size);
    return size;
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    memcpy(blocks_get_block(0) + offset, buf, size);
    return size;
}

int storage_truncate(const char *path, off_t size) { return 0; }
int storage_mknod(const char *path, int mode) { return 0; }
int storage_unlink(const char *path) { return 0; }
int storage_link(const char *from, const char *to) { return 0; }
int storage_rename(const char *from, const char *to) { return 0; }
int storage_set_time(const char *path, const struct timespec ts[2]) { return 0; }
slist_t *storage_list(const char *path) { return NULL; }

int storage_read_block(int bnum, char *buf, int offset, int size) {
    void *block = blocks_get_block(bnum);
    memcpy(buf, block + offset, size);
    return size;
}

int storage_write_block(int bnum, const char *buf, int offset, int size) {
    void *block = blocks_get_block(bnum);
    memcpy(block + offset, buf, size);
    return size;
}
