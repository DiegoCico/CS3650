#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "helpers/blocks.h"   // Helper for disk block operations
#include "helpers/bitmap.h"   // Helper for bitmap operations
#include "helpers/slist.h"    // Helper for string list manipulation

// Simple in-memory file table
typedef enum { FILE_REGULAR, FILE_DIRECTORY } file_type;

typedef struct file_entry {
    char *path;          // full path (e.g., "/" or "/hello.txt")
    mode_t mode;         // permissions and type bits
    size_t size;         // file size (for regular files)
    int block;           // block number for file data (if regular)
    file_type type;      // file type (regular or directory)
    struct timespec atime, mtime, ctime; // timestamps
    int valid;           // true if used
} file_entry_t;

#define MAX_FILES 1024
static file_entry_t fs_files[MAX_FILES];
static int fs_initialized = 0;

// Initialize file table with root and hello.txt
void fs_init() {
    if (fs_initialized) return;
    for (int i = 0; i < MAX_FILES; i++) {
        fs_files[i].valid = 0;
    }
    fs_files[0].path = strdup("/");
    fs_files[0].mode = 040755;
    fs_files[0].size = 0;
    fs_files[0].type = FILE_DIRECTORY;
    clock_gettime(CLOCK_REALTIME, &fs_files[0].atime);
    clock_gettime(CLOCK_REALTIME, &fs_files[0].mtime);
    clock_gettime(CLOCK_REALTIME, &fs_files[0].ctime);
    fs_files[0].valid = 1;
    printf("fs_init: created root directory\n");

    fs_files[1].path = strdup("/hello.txt");
    fs_files[1].mode = 0100644;
    fs_files[1].size = 6;
    fs_files[1].type = FILE_REGULAR;
    int blk = alloc_block();
    if (blk < 0) {
        perror("alloc_block failed for hello.txt");
        exit(1);
    }
    fs_files[1].block = blk;
    char *data = blocks_get_block(blk);
    strcpy(data, "hello\n");
    clock_gettime(CLOCK_REALTIME, &fs_files[1].atime);
    clock_gettime(CLOCK_REALTIME, &fs_files[1].mtime);
    clock_gettime(CLOCK_REALTIME, &fs_files[1].ctime);
    fs_files[1].valid = 1;
    printf("fs_init: created /hello.txt in block %d\n", blk);

    fs_initialized = 1;
}

// Look up a file entry by path
file_entry_t* fs_lookup(const char *path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs_files[i].valid && strcmp(fs_files[i].path, path) == 0)
            return &fs_files[i];
    }
    return NULL;
}

// Add a new file entry; allocate a block if it's a regular file
file_entry_t* fs_add(const char *path, mode_t mode, file_type type) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs_files[i].valid) {
            fs_files[i].path = strdup(path);
            fs_files[i].mode = mode;
            fs_files[i].size = 0;
            fs_files[i].type = type;
            if (type == FILE_REGULAR) {
                int blk = alloc_block();
                if (blk < 0) {
                    printf("fs_add: alloc_block failed for %s\n", path);
                    return NULL;
                }
                fs_files[i].block = blk;
                memset(blocks_get_block(blk), 0, BLOCK_SIZE);
                printf("fs_add: allocated block %d for %s\n", blk, path);
            }
            clock_gettime(CLOCK_REALTIME, &fs_files[i].atime);
            clock_gettime(CLOCK_REALTIME, &fs_files[i].mtime);
            clock_gettime(CLOCK_REALTIME, &fs_files[i].ctime);
            fs_files[i].valid = 1;
            return &fs_files[i];
        }
    }
    return NULL;
}

// Remove a file entry; free its block if needed
int fs_remove(const char *path) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs_files[i].valid && strcmp(fs_files[i].path, path) == 0) {
            if (fs_files[i].type == FILE_REGULAR)
                free_block(fs_files[i].block);
            free(fs_files[i].path);
            fs_files[i].valid = 0;
            printf("fs_remove: removed %s\n", path);
            return 0;
        }
    }
    return -ENOENT;
}

// Check if a file is an immediate child of a directory (for readdir)
int is_immediate_child(const char *dir, const char *path) {
    size_t len = strlen(dir);
    if (strcmp(dir, "/") == 0) {
        if (strcmp(path, "/") == 0) return 0;
        const char *p = path + 1;
        return (strchr(p, '/') == NULL);
    } else {
        if (strncmp(path, dir, len) != 0) return 0;
        if (strlen(path) == len) return 0;
        if (path[len] != '/') return 0;
        const char *p = path + len + 1;
        return (strchr(p, '/') == NULL);
    }
}

// FUSE callback: Check file access
int nufs_access(const char *path, int mask) {
    fs_init();
    file_entry_t* fe = fs_lookup(path);
    int ret = (fe) ? 0 : -ENOENT;
    printf("access: %s mask:%04o -> %d\n", path, mask, ret);
    return ret;
}

// FUSE callback: Get file attributes (stat)
int nufs_getattr(const char *path, struct stat *st) {
    fs_init();
    file_entry_t* fe = fs_lookup(path);
    if (!fe) {
        printf("getattr: %s -> -ENOENT\n", path);
        return -ENOENT;
    }
    memset(st, 0, sizeof(struct stat));
    st->st_uid = getuid();
    st->st_atime = fe->atime.tv_sec;
    st->st_mtime = fe->mtime.tv_sec;
    st->st_ctime = fe->ctime.tv_sec;
    if (fe->type == FILE_DIRECTORY) {
        st->st_mode = fe->mode;
        st->st_nlink = 2;
    } else {
        st->st_mode = fe->mode;
        st->st_nlink = 1;
        st->st_size = fe->size;
    }
    printf("getattr: %s -> mode:%04o size:%ld\n", path, st->st_mode, st->st_size);
    return 0;
}

// FUSE callback: List directory contents
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
    fs_init();
    file_entry_t* dir = fs_lookup(path);
    if (!dir || dir->type != FILE_DIRECTORY) {
        printf("readdir: %s -> not a directory\n", path);
        return -ENOENT;
    }
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_mode = dir->mode;
    filler(buf, ".", &st, 0);
    filler(buf, "..", &st, 0);
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs_files[i].valid) continue;
        if (is_immediate_child(path, fs_files[i].path)) {
            const char *name = strrchr(fs_files[i].path, '/');
            name = name ? name + 1 : fs_files[i].path;
            nufs_getattr(fs_files[i].path, &st);
            filler(buf, name, &st, 0);
        }
    }
    printf("readdir: %s listed\n", path);
    return 0;
}

// FUSE callback: Create a new file
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
    fs_init();
    if (fs_lookup(path)) {
        printf("mknod: %s already exists\n", path);
        return -EEXIST;
    }
    char parent[1024];
    strcpy(parent, path);
    char *last = strrchr(parent, '/');
    if (!last) return -ENOENT;
    if (last == parent)
        strcpy(parent, "/");
    else
        *last = '\0';
    file_entry_t *pdir = fs_lookup(parent);
    if (!pdir || pdir->type != FILE_DIRECTORY) return -ENOENT;
    file_entry_t *fe = fs_add(path, mode, FILE_REGULAR);
    if (!fe) {
        printf("mknod: no space for %s\n", path);
        return -ENOSPC;
    }
    printf("mknod: created file %s\n", path);
    return 0;
}

// FUSE callback: Create a new directory
int nufs_mkdir(const char *path, mode_t mode) {
    fs_init();
    if (fs_lookup(path)) {
        printf("mkdir: %s already exists\n", path);
        return -EEXIST;
    }
    char parent[1024];
    strcpy(parent, path);
    char *last = strrchr(parent, '/');
    if (!last) return -ENOENT;
    if (last == parent)
        strcpy(parent, "/");
    else
        *last = '\0';
    file_entry_t *pdir = fs_lookup(parent);
    if (!pdir || pdir->type != FILE_DIRECTORY) return -ENOENT;
    file_entry_t *fe = fs_add(path, mode, FILE_DIRECTORY);
    if (!fe) {
        printf("mkdir: no space for %s\n", path);
        return -ENOSPC;
    }
    printf("mkdir: created directory %s\n", path);
    return 0;
}

// FUSE callback: Remove a file
int nufs_unlink(const char *path) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("unlink: %s not found\n", path);
        return -ENOENT;
    }
    if (fe->type != FILE_REGULAR) {
        printf("unlink: %s is a directory\n", path);
        return -EISDIR;
    }
    int ret = fs_remove(path);
    printf("unlink: %s removed -> %d\n", path, ret);
    return ret;
}

// FUSE callback: Remove a directory (only if empty)
int nufs_rmdir(const char *path) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("rmdir: %s not found\n", path);
        return -ENOENT;
    }
    if (fe->type != FILE_DIRECTORY) {
        printf("rmdir: %s is not a directory\n", path);
        return -ENOTDIR;
    }
    for (int i = 0; i < MAX_FILES; i++) {
        if (!fs_files[i].valid) continue;
        if (is_immediate_child(path, fs_files[i].path)) {
            printf("rmdir: %s not empty\n", path);
            return -ENOTEMPTY;
        }
    }
    int ret = fs_remove(path);
    printf("rmdir: %s removed -> %d\n", path, ret);
    return ret;
}

// FUSE callback: Rename a file or directory
int nufs_rename(const char *from, const char *to) {
    fs_init();
    file_entry_t *fe = fs_lookup(from);
    if (!fe) {
        printf("rename: %s not found\n", from);
        return -ENOENT;
    }
    if (fs_lookup(to)) {
        printf("rename: %s already exists\n", to);
        return -EEXIST;
    }
    free(fe->path);
    fe->path = strdup(to);
    printf("rename: %s renamed to %s\n", from, to);
    return 0;
}

// FUSE callback: Change file permissions
int nufs_chmod(const char *path, mode_t mode) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("chmod: %s not found\n", path);
        return -ENOENT;
    }
    fe->mode = mode;
    printf("chmod: %s mode set to %04o\n", path, mode);
    return 0;
}

// FUSE callback: Adjust file size (only supports one block)
int nufs_truncate(const char *path, off_t size) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("truncate: %s not found\n", path);
        return -ENOENT;
    }
    if (fe->type != FILE_REGULAR) {
        printf("truncate: %s is a directory\n", path);
        return -EISDIR;
    }
    if (size > BLOCK_SIZE) {
        printf("truncate: %s size %ld too big\n", path, size);
        return -EFBIG;
    }
    if (size > fe->size) {
        char *data = blocks_get_block(fe->block);
        memset(data + fe->size, 0, size - fe->size);
    }
    fe->size = size;
    printf("truncate: %s size set to %ld\n", path, size);
    return 0;
}

// FUSE callback: Open a file
int nufs_open(const char *path, struct fuse_file_info *fi) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("open: %s not found\n", path);
        return -ENOENT;
    }
    printf("open: %s opened\n", path);
    return 0;
}

// FUSE callback: Read from a file
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("read: %s not found\n", path);
        return -ENOENT;
    }
    if (fe->type != FILE_REGULAR) {
        printf("read: %s is a directory\n", path);
        return -EISDIR;
    }
    if (offset >= fe->size) {
        printf("read: %s offset %ld beyond EOF\n", path, offset);
        return 0;
    }
    if (offset + size > fe->size)
        size = fe->size - offset;
    char *data = blocks_get_block(fe->block);
    memcpy(buf, data + offset, size);
    printf("read: %s read %ld bytes at offset %ld\n", path, size, offset);
    return size;
}

// FUSE callback: Write to a file (only supports one block)
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("write: %s not found\n", path);
        return -ENOENT;
    }
    if (fe->type != FILE_REGULAR) {
        printf("write: %s is a directory\n", path);
        return -EISDIR;
    }
    if (offset + size > BLOCK_SIZE) {
        printf("write: %s write exceeds block size\n", path);
        return -EFBIG;
    }
    char *data = blocks_get_block(fe->block);
    memcpy(data + offset, buf, size);
    if (offset + size > fe->size)
        fe->size = offset + size;
    printf("write: %s wrote %ld bytes at offset %ld\n", path, size, offset);
    return size;
}

// FUSE callback: Update file timestamps
int nufs_utimens(const char *path, const struct timespec ts[2]) {
    fs_init();
    file_entry_t *fe = fs_lookup(path);
    if (!fe) {
        printf("utimens: %s not found\n", path);
        return -ENOENT;
    }
    fe->atime = ts[0];
    fe->mtime = ts[1];
    printf("utimens: %s updated timestamps\n", path);
    return 0;
}

// FUSE callback: Not implementing ioctl, so return not supported
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
    printf("ioctl: %s cmd %d not supported\n", path, cmd);
    return -ENOSYS;
}

// FUSE destroy callback: clear disk image and free mapping on unmount
void nufs_destroy(void *data) {
    memset(blocks_get_block(0), 0, NUFS_SIZE);
    blocks_free();
    printf("nufs_destroy: filesystem unmounted and disk image cleared\n");
}

// Set up FUSE operations struct
void nufs_init_ops(struct fuse_operations *ops) {
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->unlink   = nufs_unlink;
    ops->rmdir    = nufs_rmdir;
    ops->rename   = nufs_rename;
    ops->chmod    = nufs_chmod;
    ops->truncate = nufs_truncate;
    ops->open     = nufs_open;
    ops->read     = nufs_read;
    ops->write    = nufs_write;
    ops->utimens  = nufs_utimens;
    ops->ioctl    = nufs_ioctl;
    ops->destroy  = nufs_destroy;
}

struct fuse_operations nufs_ops;

// Main: init disk blocks and start FUSE
int main(int argc, char *argv[]) {
    assert(argc > 2 && argc < 6);
    char *image_path = argv[argc - 1];
    printf("Starting FUSE filesystem with image: %s\n", image_path);
    blocks_init(image_path);
    nufs_init_ops(&nufs_ops);
    return fuse_main(argc, argv, &nufs_ops, NULL);
}

/* 
 * Include helper implementations so that their symbols are defined.
 */
#include "helpers/blocks.c"
#include "helpers/bitmap.c"
#include "helpers/slist.c"
