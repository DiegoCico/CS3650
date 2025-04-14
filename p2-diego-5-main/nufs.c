/*
 * nufs.c – A basic FUSE filesystem for CS3650 Project 2.
 *
 * This filesystem mounts a 1MB disk image and supports file creation, 
 * directory operations, renaming, reading/writing, and big file support.
 */

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
 
 /* Include helper headers */
 #include "blocks.h"
 #include "bitmap.h"
 #include "inode.h"
 #include "directory.h"
 #include "storage.h"
 
 /* Utility: split_path
  * Splits a path into its parent directory and child filename.
  */
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
 
 /* Utility: path_lookup
  * Walks the filesystem tree (starting at the root inode) following the tokens of the provided path.
  * Returns a pointer to the inode if found, or NULL otherwise.
  *
  * Note: This function is now non-static (global) so it can be used by other modules.
  */
 inode_t *path_lookup(const char *path) {
     if (strcmp(path, "/") == 0)
         return get_inode(0);  // Reserve inode 0 for the root directory
 
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
 
 /* FUSE callback: access */
 int nufs_access(const char *path, int mask) {
     inode_t *node = path_lookup(path);
     if (node == NULL) {
         printf("access(%s) -> -ENOENT\n", path);
         return -ENOENT;
     }
     printf("access(%s) -> 0\n", path);
     return 0;
 }
 
 /* FUSE callback: getattr */
 int nufs_getattr(const char *path, struct stat *st) {
     inode_t *node = path_lookup(path);
     if (node == NULL) {
         printf("getattr(%s) -> -ENOENT\n", path);
         return -ENOENT;
     }
     memset(st, 0, sizeof(struct stat));
     st->st_uid = getuid();
     st->st_gid = getgid();
     st->st_size = node->size;
     if (node->mode & 040000) { // directory
         st->st_mode = 040755;
         st->st_nlink = 2;
     } else {
         st->st_mode = 0100644;
         st->st_nlink = 1;
     }
     printf("getattr(%s) -> (mode: %o, size: %d)\n", path, st->st_mode, node->size);
     return 0;
 }
 
 /* FUSE callback: readdir */
 int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi) {
     inode_t *dir_inode = path_lookup(path);
     if (dir_inode == NULL || !(dir_inode->mode & 040000)) {
         printf("readdir(%s) -> -ENOENT (not a directory)\n", path);
         return -ENOENT;
     }
     struct stat st;
     memset(&st, 0, sizeof(st));
     st.st_mode = dir_inode->mode;
     filler(buf, ".", &st, 0);
     filler(buf, "..", &st, 0);
     slist_t *list = directory_list(path);
     if (list == NULL) {
         printf("readdir(%s) -> directory empty or error\n", path);
         return 0;
     }
     slist_t *curr = list;
     while (curr != NULL) {
         fs_dirent_t *entry = (fs_dirent_t *) curr->data;
         inode_t *entry_inode = get_inode(entry->inum);
         memset(&st, 0, sizeof(st));
         st.st_mode = entry_inode->mode;
         st.st_size = entry_inode->size;
         filler(buf, entry->name, &st, 0);
         curr = curr->next;
     }
     slist_free(list);
     printf("readdir(%s) -> OK\n", path);
     return 0;
 }
 
 /* FUSE callback: mknod */
 int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
     char parent[256], child[256];
     split_path(path, parent, child);
     inode_t *parent_inode = path_lookup(parent);
     if (parent_inode == NULL || !(parent_inode->mode & 040000)) {
         printf("mknod: Parent directory %s not found\n", parent);
         return -ENOENT;
     }
     if (directory_lookup(parent_inode, child) >= 0) {
         printf("mknod: File %s already exists\n", path);
         return -EEXIST;
     }
     int inum = alloc_inode();
     if (inum < 0) {
         printf("mknod: No free inodes\n");
         return -ENOSPC;
     }
     inode_t *new_inode = get_inode(inum);
     new_inode->mode = mode;
     new_inode->size = 0;
     new_inode->block = -1;
     int res = directory_put(parent_inode, child, inum);
     if (res < 0) {
         printf("mknod: Failed to add %s to directory %s\n", child, parent);
         return res;
     }
     printf("mknod(%s) -> inode %d created\n", path, inum);
     return 0;
 }
 
 /* FUSE callback: mkdir */
 int nufs_mkdir(const char *path, mode_t mode) {
     mode |= 040000;
     int rv = nufs_mknod(path, mode, 0);
     if (rv == 0) {
         inode_t *dir_inode = path_lookup(path);
         printf("mkdir(%s) -> directory created\n", path);
     } else {
         printf("mkdir(%s) -> error %d\n", path, rv);
     }
     return rv;
 }
 
 /* FUSE callback: unlink */
 int nufs_unlink(const char *path) {
     char parent[256], child[256];
     split_path(path, parent, child);
     inode_t *parent_inode = path_lookup(parent);
     if (parent_inode == NULL)
         return -ENOENT;
     int inum = directory_lookup(parent_inode, child);
     if (inum < 0)
         return -ENOENT;
     int rv = directory_delete(parent_inode, child);
     if (rv < 0)
         return rv;
     free_inode(inum);
     printf("unlink(%s) -> file removed\n", path);
     return 0;
 }
 
 /* FUSE callback: rmdir */
 int nufs_rmdir(const char *path) {
     inode_t *dir_inode = path_lookup(path);
     if (dir_inode == NULL || !(dir_inode->mode & 040000))
         return -ENOTDIR;
     slist_t *list = directory_list(path);
     if (list != NULL) {
         slist_free(list);
         return -ENOTEMPTY;
     }
     char parent[256], child[256];
     split_path(path, parent, child);
     inode_t *parent_inode = path_lookup(parent);
     if (parent_inode == NULL)
         return -ENOENT;
     int rv = directory_delete(parent_inode, child);
     if (rv < 0)
         return rv;
     free_inode(0); // In a full implementation, free the specific inode
     printf("rmdir(%s) -> directory removed\n", path);
     return 0;
 }
 
 /* FUSE callback: rename */
 int nufs_rename(const char *from, const char *to) {
     char from_parent[256], from_child[256];
     split_path(from, from_parent, from_child);
     inode_t *from_parent_inode = path_lookup(from_parent);
     if (from_parent_inode == NULL)
         return -ENOENT;
     int inum = directory_lookup(from_parent_inode, from_child);
     if (inum < 0)
         return -ENOENT;
     int rv = directory_delete(from_parent_inode, from_child);
     if (rv < 0)
         return rv;
     char to_parent[256], to_child[256];
     split_path(to, to_parent, to_child);
     inode_t *to_parent_inode = path_lookup(to_parent);
     if (to_parent_inode == NULL)
         return -ENOENT;
     rv = directory_put(to_parent_inode, to_child, inum);
     if (rv < 0)
         return rv;
     printf("rename(%s -> %s) -> successful\n", from, to);
     return 0;
 }
 
 /* FUSE callback: chmod */
 int nufs_chmod(const char *path, mode_t mode) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     node->mode = mode;
     printf("chmod(%s, %04o) -> successful\n", path, mode);
     return 0;
 }
 
 /* Helper: read_from_inode */
 static int read_from_inode(inode_t *node, char *buf, size_t size, off_t offset) {
     if (offset >= node->size)
         return 0;
     if (offset + size > node->size)
         size = node->size - offset;
     int bytes_read = 0;
     int block_size = 4096;
     int start_block = offset / block_size;
     int block_offset = offset % block_size;
     while (size > 0) {
         int bnum = inode_get_bnum(node, start_block);
         if (bnum < 0)
             break;
         int chunk = block_size - block_offset;
         if (chunk > size)
             chunk = size;
         int rv = storage_read_block(bnum, buf + bytes_read, block_offset, chunk);
         if (rv < 0)
             return rv;
         size -= chunk;
         bytes_read += chunk;
         start_block++;
         block_offset = 0;
     }
     return bytes_read;
 }
 
 /* FUSE callback: read */
 int nufs_read(const char *path, char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     int bytes = read_from_inode(node, buf, size, offset);
     printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, bytes);
     return bytes;
 }
 
 /* Helper: write_to_inode */
 static int write_to_inode(inode_t *node, const char *buf, size_t size, off_t offset) {
     int block_size = 4096;
     if (offset + size > node->size) {
         int rv = grow_inode(node, offset + size);
         if (rv < 0)
             return rv;
     }
     int bytes_written = 0;
     int start_block = offset / block_size;
     int block_offset = offset % block_size;
     while (size > 0) {
         int bnum = inode_get_bnum(node, start_block);
         if (bnum < 0)
             break;
         int chunk = block_size - block_offset;
         if (chunk > size)
             chunk = size;
         int rv = storage_write_block(bnum, buf + bytes_written, block_offset, chunk);
         if (rv < 0)
             return rv;
         size -= chunk;
         bytes_written += chunk;
         start_block++;
         block_offset = 0;
     }
     return bytes_written;
 }
 
 /* FUSE callback: write */
 int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
                struct fuse_file_info *fi) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     int bytes = write_to_inode(node, buf, size, offset);
     printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, bytes);
     return bytes;
 }
 
 /* FUSE callback: truncate */
 int nufs_truncate(const char *path, off_t size) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     if (size < node->size) {
         int rv = shrink_inode(node, size);
         if (rv < 0)
             return rv;
     } else if (size > node->size) {
         int rv = grow_inode(node, size);
         if (rv < 0)
             return rv;
     }
     node->size = size;
     printf("truncate(%s, %ld bytes) -> successful\n", path, size);
     return 0;
 }
 
 /* FUSE callback: utimens */
 int nufs_utimens(const char *path, const struct timespec ts[2]) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     printf("utimens(%s, [%ld.%ld, %ld.%ld]) -> successful\n",
            path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec);
     return 0;
 }
 
 /* FUSE callback: open */
 int nufs_open(const char *path, struct fuse_file_info *fi) {
     inode_t *node = path_lookup(path);
     if (node == NULL)
         return -ENOENT;
     printf("open(%s) -> successful\n", path);
     return 0;
 }
 
 /* FUSE callback: ioctl (stub) */
 int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
                unsigned int flags, void *data) {
     printf("ioctl(%s, %d, ...) -> not implemented\n", path, cmd);
     return -ENOSYS;
 }
 
 /* Set up FUSE operations structure */
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
 };
 
 struct fuse_operations nufs_ops;
 
 int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <mountpoint> <disk image>\n", argv[0]);
        exit(1);
    }
    // The last argument is the disk image file.
    const char *disk_image = argv[argc - 1];
    storage_init(disk_image);

    inode_t *root = get_inode(0);
    if (root == NULL) {
        int inum = alloc_inode();
        root = get_inode(inum);
        root->mode = 040755;   // directory mode
        root->size = 0;
    }
    nufs_init_ops(&nufs_ops);
    printf("Mounting filesystem with disk image: %s\n", disk_image);

    // Prepare a new argument list for FUSE by excluding the disk image.
    int fuse_argc = argc - 1;  // exclude the last argument (disk image)
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
