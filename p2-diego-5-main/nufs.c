#include <assert.h>
#include <dirent.h>  
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "blocks.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"
#include "storage.h"

/* global struct to register fuse operations */
struct fuse_operations nufs_ops;

/* splits a full path into parent dir and child filename */
static void split_path(const char *path, char *parent, char *child) {
    const char *p = path;
    if (p[0] == '/')
        p++;
    const char *last_slash = strrchr(p, '/');
    if (last_slash == NULL) {
        strcpy(parent, "/");
        strcpy(child, p);
    } else {
        size_t len = last_slash - p;
        if (len == 0) {
            strcpy(parent, "/");
        } else {
            strncpy(parent, p, len);
            parent[len] = '\0';
            char temp[256];
            snprintf(temp, sizeof(temp), "/%s", parent);
            strcpy(parent, temp);
        }
        strcpy(child, last_slash + 1);
    }
}

/* traverses the filesystem to find inode of a given path */
inode_t *path_lookup(const char *path) {
    if (strcmp(path, "/") == 0)
        return get_inode(0);  
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy));
    path_copy[sizeof(path_copy)-1] = '\0';
    char *token;
    inode_t *curr = get_inode(0);
    token = strtok(path_copy, "/");
    while (token != NULL && curr != NULL) {
        int child_inum = directory_lookup(curr, token);
        if (child_inum < 0)
            return NULL;
        curr = get_inode(child_inum);
        token = strtok(NULL, "/");
    }
    return curr;
}

/* checks if path exists */
int nufs_access(const char *path, int mask) {
    inode_t *node = path_lookup(path);
    if (node == NULL) return -ENOENT;
    return 0;
}

/* gets file metadata like size, mode, etc */
int nufs_getattr(const char *path, struct stat *st) {
    inode_t *node = path_lookup(path);
    if (node == NULL) return -ENOENT;
    memset(st, 0, sizeof(struct stat));
    st->st_uid = getuid();   // sets file owner to current user
    st->st_gid = getgid();  // sets file group to current user group
    st->st_size = node->size; // sets file size from inode
    st->st_mode = (node->mode & 040000) ? 040755 : 0100644;  // if it’s a dir, use dir perms else use regular file perms
    st->st_nlink = (node->mode & 040000) ? 2 : 1;  // sets link count 2 for dirs 1 for files
    return 0;
}

/* lists all files in a directory */
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
    inode_t *dir_inode = path_lookup(path);
    if (!dir_inode || !(dir_inode->mode & 040000)) return -ENOENT;

    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_mode = dir_inode->mode;
    filler(buf, ".", &st, 0);
    filler(buf, "..", &st, 0);

    slist_t *list = directory_list(path);
    slist_t *curr = list;
    while (curr) {
        fs_dirent_t *entry = (fs_dirent_t *) curr->data;
        inode_t *entry_inode = get_inode(entry->inum);
        memset(&st, 0, sizeof(st));
        st.st_mode = entry_inode->mode;
        st.st_size = entry_inode->size;
        filler(buf, entry->name, &st, 0);
        curr = curr->next;
    }
    slist_free(list);
    return 0;
}

/* creates a file with given path and mode */
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
    char parent[256], child[256];
    split_path(path, parent, child);
    inode_t *parent_inode = path_lookup(parent);
    if (!parent_inode || !(parent_inode->mode & 040000)) return -ENOENT;
    if (directory_lookup(parent_inode, child) >= 0) return -EEXIST;

    int inum = alloc_inode();
    if (inum < 0) return -ENOSPC;

    inode_t *new_inode = get_inode(inum);
    new_inode->mode = mode;
    new_inode->size = 0;
    new_inode->block = -1;
    return directory_put(parent_inode, child, inum);
}

/* creates a directory by calling mknod with dir flag */
int nufs_mkdir(const char *path, mode_t mode) {
    mode |= 040000; // set directory bit in mode
    return nufs_mknod(path, mode, 0);
}

/* deletes a file by removing it from directory and freeing inode */
int nufs_unlink(const char *path) {
    char parent[256], child[256];
    split_path(path, parent, child);
    inode_t *parent_inode = path_lookup(parent);
    if (!parent_inode) return -ENOENT;
    int inum = directory_lookup(parent_inode, child);
    if (inum < 0) return -ENOENT;

    directory_delete(parent_inode, child);
    free_inode(inum);
    return 0;
}
int nufs_rmdir(const char *path) {
    inode_t *dir_inode = path_lookup(path);
    if (!dir_inode || !(dir_inode->mode & 040000)) return -ENOTDIR;

    slist_t *list = directory_list(path);
    if (list) {
        slist_free(list);
        return -ENOTEMPTY;
    }

    char parent[256];
    char child[256];
    split_path(path, parent, child);

    inode_t *parent_inode = path_lookup(parent);
    if (!parent_inode) return -ENOENT;

    int inum = directory_lookup(parent_inode, child); 
    int rv = directory_delete(parent_inode, child);
    if (rv < 0) return rv;

    free_inode(inum); 
    return 0;
}


/* renames or moves a file from one path to another */
int nufs_rename(const char *from, const char *to) {
    char from_parent[256], from_child[256];
    split_path(from, from_parent, from_child);
    inode_t *from_parent_inode = path_lookup(from_parent);
    if (!from_parent_inode) return -ENOENT;

    int inum = directory_lookup(from_parent_inode, from_child);
    if (inum < 0) return -ENOENT;

    directory_delete(from_parent_inode, from_child);

    char to_parent[256], to_child[256];
    split_path(to, to_parent, to_child);
    inode_t *to_parent_inode = path_lookup(to_parent);
    if (!to_parent_inode) return -ENOENT;

    return directory_put(to_parent_inode, to_child, inum);
}

/* changes the permissions of a file */
int nufs_chmod(const char *path, mode_t mode) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    node->mode = mode;
    return 0;
}

/* reads data from an inode into buffer */
static int read_from_inode(inode_t *node, char *buf, size_t size, off_t offset) {
    if (offset >= node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;

    int bytes_read = 0;
    int block_size = 4096;
    int start_block = offset / block_size;
    int block_offset = offset % block_size;

    while (size > 0) {
        int bnum = inode_get_bnum(node, start_block);
        if (bnum < 0) break;

        int chunk = block_size - block_offset;
        if (chunk > size) chunk = size;

        int rv = storage_read_block(bnum, buf + bytes_read, block_offset, chunk);
        if (rv < 0) return rv;

        size -= chunk;
        bytes_read += chunk;
        start_block++;
        block_offset = 0;
    }
    return bytes_read;
}

/* reads from file using inode logic */
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    return read_from_inode(node, buf, size, offset);
}

/* writes buffer into inode starting at offset */
static int write_to_inode(inode_t *node, const char *buf, size_t size, off_t offset) {
    int block_size = 4096;
    if (offset + size > node->size) {
        int rv = grow_inode(node, offset + size);
        if (rv < 0) return rv;
    }

    int bytes_written = 0;
    int start_block = offset / block_size;
    int block_offset = offset % block_size;

    while (size > 0) {
        int bnum = inode_get_bnum(node, start_block);
        if (bnum < 0) break;

        int chunk = block_size - block_offset;
        if (chunk > size) chunk = size;

        int rv = storage_write_block(bnum, buf + bytes_written, block_offset, chunk);
        if (rv < 0) return rv;

        size -= chunk;
        bytes_written += chunk;
        start_block++;
        block_offset = 0;
    }
    return bytes_written;
}

/* writes to a file at offset */
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    return write_to_inode(node, buf, size, offset);
}

/* changes file size, shrinnk orit grow s */
int nufs_truncate(const char *path, off_t size) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    if (size < node->size) return shrink_inode(node, size);
    if (size > node->size) return grow_inode(node, size);
    node->size = size;
    return 0;
}

/* updates file timestamps */
int nufs_utimens(const char *path, const struct timespec ts[2]) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    return 0;
}

/* opens a file */
int nufs_open(const char *path, struct fuse_file_info *fi) {
    inode_t *node = path_lookup(path);
    if (!node) return -ENOENT;
    return 0;
}

/* stub for ioctl */
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
    return -ENOSYS;
}

/* sets up all supported fuse operations */
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
}

/* mounts the fs and hands over control to fuse */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mountpoint> <disk image>\n", argv[0]);
        exit(1);
    }

    const char *disk_image = argv[argc - 1];
    storage_init(disk_image); // load disk image

    inode_t *root = get_inode(0);
    if (!root) {
        int inum = alloc_inode();
        root = get_inode(inum);
        root->mode = 040755;   // mark as directory
        root->size = 0;
    }

    nufs_init_ops(&nufs_ops);
    printf("Mounting filesystem with disk image: %s\n", disk_image);

    // remove disk image arg before passing to fuse
    int fuse_argc = argc - 1;
    char **fuse_argv = malloc(sizeof(char*) * fuse_argc);
    if (!fuse_argv) {
        perror("malloc");
        exit(1);
    }
    for (int i = 0; i < fuse_argc; i++) {
        fuse_argv[i] = argv[i];
    }

    int ret = fuse_main(fuse_argc, fuse_argv, &nufs_ops, NULL);
    free(fuse_argv);
    return ret;
}
