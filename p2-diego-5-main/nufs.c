// nufs.c
#define _GNU_SOURCE
#define FUSE_USE_VERSION 26

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * -------------
 * Data Structures
 * -------------
 *
 * We represent our file system as an in‑memory tree. Each node (fs_node)
 * represents either a file or a directory. For files, the data is stored in a
 * dynamically allocated char buffer. For directories, we maintain a dynamic
 * array of pointers to child fs_nodes.
 */

typedef struct fs_node {
    char *name;                // name of the node (e.g., "foo.txt")
    char *path;                // full path (e.g., "/foo.txt" or "/dir/bar")
    int is_dir;                // 1 if directory, 0 if file
    mode_t mode;               // permission bits
    off_t size;                // file size (or arbitrary 4096 for dirs)
    char *data;                // file contents, if any (NULL for directories)
    struct timespec atime;     // access time
    struct timespec mtime;     // modification time
    struct timespec ctime;     // change time

    // For directories: dynamic array of pointers to children.
    struct fs_node **children;
    int num_children;
} fs_node;

// Global pointer to the root of our file system.
static fs_node *fs_root = NULL;

// Global lock for thread safety.
static pthread_mutex_t fs_lock = PTHREAD_MUTEX_INITIALIZER;

// Global disk image filename (set from main’s arguments).
char fs_disk_path[PATH_MAX] = {0};

/*
 * ------------------------------
 * In-Memory FS Helper Functions
 * ------------------------------
 */

static fs_node *fs_create_node(const char *name, int is_dir, mode_t mode) {
    fs_node *node = malloc(sizeof(fs_node));
    if (!node)
        return NULL;
    node->name = strdup(name);
    node->path = NULL;  // will be updated later
    node->is_dir = is_dir;
    node->mode = mode;
    node->size = is_dir ? 4096 : 0;
    node->data = NULL;
    clock_gettime(CLOCK_REALTIME, &node->atime);
    clock_gettime(CLOCK_REALTIME, &node->mtime);
    clock_gettime(CLOCK_REALTIME, &node->ctime);
    node->children = NULL;
    node->num_children = 0;
    return node;
}

static void fs_free_node(fs_node *node) {
    if (!node)
        return;
    free(node->name);
    free(node->path);
    if (node->data)
        free(node->data);
    for (int i = 0; i < node->num_children; i++) {
        fs_free_node(node->children[i]);
    }
    free(node->children);
    free(node);
}

// Add a child node to a directory.
static int fs_add_child(fs_node *parent, fs_node *child) {
    if (!parent || !parent->is_dir)
        return -ENOTDIR;
    fs_node **tmp = realloc(parent->children, sizeof(fs_node *) * (parent->num_children + 1));
    if (!tmp)
        return -ENOMEM;
    parent->children = tmp;
    parent->children[parent->num_children] = child;
    parent->num_children++;
    return 0;
}

// Look for a child by name in a directory.
static fs_node *fs_find_in_dir(fs_node *dir, const char *name) {
    if (!dir || !dir->is_dir)
        return NULL;
    for (int i = 0; i < dir->num_children; i++) {
        if (strcmp(dir->children[i]->name, name) == 0)
            return dir->children[i];
    }
    return NULL;
}

// Look up a node by its path.
// Returns fs_root for "/" and traverses subdirectories for e.g. "/foo/bar".
static fs_node *fs_lookup(const char *path) {
    if (!path || path[0] != '/')
        return NULL;
    if (strcmp(path, "/") == 0)
        return fs_root;
    char *dup = strdup(path);
    if (!dup)
        return NULL;
    fs_node *cur = fs_root;
    char *token = strtok(dup, "/");
    while (token) {
        cur = fs_find_in_dir(cur, token);
        if (!cur) {
            free(dup);
            return NULL;
        }
        token = strtok(NULL, "/");
    }
    free(dup);
    return cur;
}

// Remove a child from a directory.
static int fs_remove_child(fs_node *parent, const char *name) {
    if (!parent || !parent->is_dir)
        return -ENOTDIR;
    int found = 0;
    for (int i = 0; i < parent->num_children; i++) {
        if (strcmp(parent->children[i]->name, name) == 0) {
            found = 1;
            fs_free_node(parent->children[i]);
            for (int j = i; j < parent->num_children - 1; j++)
                parent->children[j] = parent->children[j+1];
            parent->num_children--;
            if (parent->num_children > 0)
                parent->children = realloc(parent->children, sizeof(fs_node *) * parent->num_children);
            else {
                free(parent->children);
                parent->children = NULL;
            }
            break;
        }
    }
    return found ? 0 : -ENOENT;
}

// Update the full path for a node and recursively for its children.
static int fs_update_path(fs_node *node, const char *new_full_path) {
    if (!node)
        return -EINVAL;
    free(node->path);
    node->path = strdup(new_full_path);
    if (!node->path)
        return -ENOMEM;
    if (node->is_dir) {
        for (int i = 0; i < node->num_children; i++) {
            char child_path[PATH_MAX];
            snprintf(child_path, PATH_MAX, "%s/%s", new_full_path, node->children[i]->name);
            fs_update_path(node->children[i], child_path);
        }
    }
    return 0;
}

/*
 * -------------------------------------
 * Serialization / Persistence Functions
 * -------------------------------------
 *
 * We persist our in-memory file system by doing a pre-order traversal of the fs_node
 * tree and writing each node's type, full path, mode, size, and (if file) data to the disk image.
 */

static void fs_collect_nodes(fs_node *node, fs_node ***list, int *count) {
    if (!node)
        return;
    *list = realloc(*list, sizeof(fs_node *) * ((*count) + 1));
    (*list)[*count] = node;
    (*count)++;
    if (node->is_dir) {
        for (int i = 0; i < node->num_children; i++) {
            fs_collect_nodes(node->children[i], list, count);
        }
    }
}

static int fs_save(void) {
    pthread_mutex_lock(&fs_lock);
    FILE *fp = fopen(fs_disk_path, "wb");
    if (!fp) {
        pthread_mutex_unlock(&fs_lock);
        perror("fs_save fopen");
        return -1;
    }
    fs_node **nodes = NULL;
    int count = 0;
    fs_collect_nodes(fs_root, &nodes, &count);
    fwrite(&count, sizeof(int), 1, fp);
    for (int i = 0; i < count; i++) {
        fs_node *node = nodes[i];
        char type = node->is_dir ? 'D' : 'F';
        fwrite(&type, sizeof(char), 1, fp);
        int pathlen = node->path ? strlen(node->path) + 1 : 0;
        fwrite(&pathlen, sizeof(int), 1, fp);
        if (pathlen > 0)
            fwrite(node->path, sizeof(char), pathlen, fp);
        fwrite(&node->mode, sizeof(mode_t), 1, fp);
        fwrite(&node->size, sizeof(off_t), 1, fp);
        if (!node->is_dir && node->size > 0 && node->data)
            fwrite(node->data, sizeof(char), node->size, fp);
    }
    free(nodes);
    fclose(fp);
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int fs_load(void) {
    pthread_mutex_lock(&fs_lock);
    FILE *fp = fopen(fs_disk_path, "rb");
    if (!fp) {
        // No disk image found; create a new file system with a root directory.
        fs_root = fs_create_node("/", 1, 0755);
        fs_update_path(fs_root, "/");
        pthread_mutex_unlock(&fs_lock);
        return 0;
    }
    int count = 0;
    fread(&count, sizeof(int), 1, fp);
    fs_node **nodes = malloc(sizeof(fs_node *) * count);
    for (int i = 0; i < count; i++) {
        char type;
        fread(&type, sizeof(char), 1, fp);
        int pathlen;
        fread(&pathlen, sizeof(int), 1, fp);
        char pathbuf[PATH_MAX];
        if (pathlen > 0) {
            fread(pathbuf, sizeof(char), pathlen, fp);
        } else {
            pathbuf[0] = '\0';
        }
        mode_t mode;
        fread(&mode, sizeof(mode_t), 1, fp);
        off_t size;
        fread(&size, sizeof(off_t), 1, fp);
        fs_node *node = fs_create_node(basename(pathbuf), (type == 'D'), mode);
        node->size = size;
        if (type == 'F' && size > 0) {
            node->data = malloc(size);
            fread(node->data, sizeof(char), size, fp);
        }
        node->path = strdup(pathbuf);
        nodes[i] = node;
    }
    fclose(fp);
    // Reconstruct parent-child relationships.
    fs_root = NULL;
    for (int i = 0; i < count; i++) {
        if (strcmp(nodes[i]->path, "/") == 0) {
            fs_root = nodes[i];
            break;
        }
    }
    for (int i = 0; i < count; i++) {
        if (strcmp(nodes[i]->path, "/") == 0)
            continue;
        char parent_path[PATH_MAX];
        strcpy(parent_path, nodes[i]->path);
        char *last_slash = strrchr(parent_path, '/');
        if (!last_slash)
            continue;
        if (last_slash == parent_path)
            strcpy(parent_path, "/");
        else
            *last_slash = '\0';
        fs_node *parent = fs_lookup(parent_path);
        if (parent)
            fs_add_child(parent, nodes[i]);
    }
    free(nodes);
    if (!fs_root) {
        fs_root = fs_create_node("/", 1, 0755);
        fs_update_path(fs_root, "/");
    }
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static void fs_free_tree(fs_node *node) {
    if (!node)
        return;
    fs_free_node(node);
}

/*
 * -------------------------
 * FUSE Callback Definitions
 * -------------------------
 */

static int nufs_access(const char *path, int mask) {
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    pthread_mutex_unlock(&fs_lock);
    if (!node)
        return -ENOENT;
    return 0;
}

static int nufs_getattr(const char *path, struct stat *st) {
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    pthread_mutex_unlock(&fs_lock);
    if (!node)
        return -ENOENT;
    memset(st, 0, sizeof(struct stat));
    if (node->is_dir) {
        st->st_mode = S_IFDIR | node->mode;
        st->st_nlink = 2 + node->num_children;
        st->st_size = 4096;
    } else {
        st->st_mode = S_IFREG | node->mode;
        st->st_nlink = 1;
        st->st_size = node->size;
    }
    st->st_atime = node->atime.tv_sec;
    st->st_mtime = node->mtime.tv_sec;
    st->st_ctime = node->ctime.tv_sec;
    return 0;
}

static int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;
    pthread_mutex_lock(&fs_lock);
    fs_node *dir = fs_lookup(path);
    if (!dir) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    if (!dir->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOTDIR;
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (int i = 0; i < dir->num_children; i++) {
        filler(buf, dir->children[i]->name, NULL, 0);
    }
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
    (void) rdev;
    pthread_mutex_lock(&fs_lock);
    char path_copy[PATH_MAX];
    strncpy(path_copy, path, PATH_MAX);
    char *dir_name = dirname(path_copy);
    fs_node *parent = fs_lookup(dir_name);
    if (!parent || !parent->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    char path_copy2[PATH_MAX];
    strncpy(path_copy2, path, PATH_MAX);
    char *base = basename(path_copy2);
    if (fs_find_in_dir(parent, base)) {
        pthread_mutex_unlock(&fs_lock);
        return -EEXIST;
    }
    fs_node *node = fs_create_node(base, 0, mode);
    node->size = 0;
    node->data = NULL;
    char full_path[PATH_MAX];
    if (strcmp(dir_name, "/") == 0)
        snprintf(full_path, PATH_MAX, "/%s", base);
    else
        snprintf(full_path, PATH_MAX, "%s/%s", dir_name, base);
    fs_update_path(node, full_path);
    int ret = fs_add_child(parent, node);
    pthread_mutex_unlock(&fs_lock);
    return ret;
}

static int nufs_mkdir(const char *path, mode_t mode) {
    pthread_mutex_lock(&fs_lock);
    char path_copy[PATH_MAX];
    strncpy(path_copy, path, PATH_MAX);
    char *dir_name = dirname(path_copy);
    fs_node *parent = fs_lookup(dir_name);
    if (!parent || !parent->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    char path_copy2[PATH_MAX];
    strncpy(path_copy2, path, PATH_MAX);
    char *base = basename(path_copy2);
    if (fs_find_in_dir(parent, base)) {
        pthread_mutex_unlock(&fs_lock);
        return -EEXIST;
    }
    fs_node *node = fs_create_node(base, 1, mode);
    node->size = 4096;
    fs_update_path(node, path);
    int ret = fs_add_child(parent, node);
    pthread_mutex_unlock(&fs_lock);
    return ret;
}

static int nufs_unlink(const char *path) {
    pthread_mutex_lock(&fs_lock);
    char path_copy[PATH_MAX];
    strncpy(path_copy, path, PATH_MAX);
    char *dir_name = dirname(path_copy);
    fs_node *parent = fs_lookup(dir_name);
    if (!parent) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    char path_copy2[PATH_MAX];
    strncpy(path_copy2, path, PATH_MAX);
    char *base = basename(path_copy2);
    fs_node *node = fs_find_in_dir(parent, base);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    if (node->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -EISDIR;
    }
    int ret = fs_remove_child(parent, base);
    pthread_mutex_unlock(&fs_lock);
    return ret;
}

static int nufs_rename(const char *from, const char *to) {
    pthread_mutex_lock(&fs_lock);
    fs_node *src = fs_lookup(from);
    if (!src) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    // Get source parent.
    char from_copy[PATH_MAX], to_copy[PATH_MAX];
    strncpy(from_copy, from, PATH_MAX);
    fs_node *src_parent = fs_lookup(dirname(from_copy));
    if (!src_parent) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    strncpy(to_copy, to, PATH_MAX);
    fs_node *tgt_parent = fs_lookup(dirname(to_copy));
    if (!tgt_parent || !tgt_parent->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    char to_copy2[PATH_MAX];
    strncpy(to_copy2, to, PATH_MAX);
    char *newname = basename(to_copy2);
    if (fs_find_in_dir(tgt_parent, newname)) {
        pthread_mutex_unlock(&fs_lock);
        return -EEXIST;
    }
    // Remove src from its parent's children.
    int removed = 0;
    for (int i = 0; i < src_parent->num_children; i++) {
        if (strcmp(src_parent->children[i]->name, src->name) == 0) {
            for (int j = i; j < src_parent->num_children - 1; j++)
                src_parent->children[j] = src_parent->children[j+1];
            src_parent->num_children--;
            removed = 1;
            break;
        }
    }
    if (!removed) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    free(src->name);
    src->name = strdup(newname);
    char new_full_path[PATH_MAX];
    if (strcmp(tgt_parent->path, "/") == 0)
        snprintf(new_full_path, PATH_MAX, "/%s", newname);
    else
        snprintf(new_full_path, PATH_MAX, "%s/%s", tgt_parent->path, newname);
    fs_update_path(src, new_full_path);
    fs_add_child(tgt_parent, src);
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int nufs_chmod(const char *path, mode_t mode) {
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    node->mode = mode;
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int nufs_truncate(const char *path, off_t size) {
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    if (node->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -EISDIR;
    }
    char *newdata = realloc(node->data, size);
    if (!newdata && size > 0) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOMEM;
    }
    if (size > node->size) {
        memset(newdata + node->size, 0, size - node->size);
    }
    node->data = newdata;
    node->size = size;
    clock_gettime(CLOCK_REALTIME, &node->mtime);
    clock_gettime(CLOCK_REALTIME, &node->ctime);
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int nufs_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    pthread_mutex_unlock(&fs_lock);
    if (!node)
        return -ENOENT;
    return 0;
}

static int nufs_read(const char *path, char *buf, size_t size, off_t offset,
                     struct fuse_file_info *fi) {
    (void) fi;
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    if (node->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -EISDIR;
    }
    if ((size_t)offset > node->size) {
        pthread_mutex_unlock(&fs_lock);
        return 0;
    }
    size_t avail = node->size - offset;
    size_t to_copy = size < avail ? size : avail;
    memcpy(buf, node->data + offset, to_copy);
    clock_gettime(CLOCK_REALTIME, &node->atime);
    pthread_mutex_unlock(&fs_lock);
    return to_copy;
}

static int nufs_write(const char *path, const char *buf, size_t size,
                      off_t offset, struct fuse_file_info *fi) {
    (void) fi;
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    if (node->is_dir) {
        pthread_mutex_unlock(&fs_lock);
        return -EISDIR;
    }
    off_t end_offset = offset + size;
    if (end_offset > node->size) {
        int ret = nufs_truncate(path, end_offset);
        if (ret < 0) {
            pthread_mutex_unlock(&fs_lock);
            return ret;
        }
        node = fs_lookup(path);
    }
    memcpy(node->data + offset, buf, size);
    if (offset + size > node->size)
        node->size = offset + size;
    clock_gettime(CLOCK_REALTIME, &node->mtime);
    clock_gettime(CLOCK_REALTIME, &node->ctime);
    pthread_mutex_unlock(&fs_lock);
    return size;
}

static int nufs_utimens(const char *path, const struct timespec ts[2]) {
    pthread_mutex_lock(&fs_lock);
    fs_node *node = fs_lookup(path);
    if (!node) {
        pthread_mutex_unlock(&fs_lock);
        return -ENOENT;
    }
    node->atime = ts[0];
    node->mtime = ts[1];
    pthread_mutex_unlock(&fs_lock);
    return 0;
}

static int nufs_ioctl(const char *path, int cmd, void *arg,
                      struct fuse_file_info *fi, unsigned int flags,
                      void *data) {
    (void) path; (void) cmd; (void) arg; (void) fi; (void) flags; (void) data;
    return -ENOTTY;
}

/*
 * ----------------------
 * FUSE Destroy Callback
 * ----------------------
 *
 * Called when the FS is unmounted. Save the FS to disk.
 */
static void nufs_destroy(void *private_data) {
    (void) private_data;
    fs_save();
    fs_free_tree(fs_root);
    fs_root = NULL;
}

/*
 * ------------------------
 * FUSE Operations Setup
 * ------------------------
 */

static void nufs_init_ops(struct fuse_operations *ops) {
    memset(ops, 0, sizeof(struct fuse_operations));
    ops->access   = nufs_access;
    ops->getattr  = nufs_getattr;
    ops->readdir  = nufs_readdir;
    ops->mknod    = nufs_mknod;
    ops->mkdir    = nufs_mkdir;
    ops->unlink   = nufs_unlink;
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

/*
 * --------------------------------
 * Main Function and Argument Cleanup
 * --------------------------------
 *
 * Note: The provided Makefile calls:
 *    ./nufs -s -f mnt data.nufs
 *
 * We must remove the disk image (last argument) and filter out the "-f" flag,
 * then append "-o nonempty" so FUSE accepts mounting over a non-empty directory.
 */

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [FUSE options] MOUNTPOINT DISK_IMAGE\n", argv[0]);
        exit(1);
    }
    // Save disk image filename from last argument.
    strncpy(fs_disk_path, argv[argc - 1], PATH_MAX);
    argv[argc - 1] = NULL;
    argc--;

    // Build a new argv array filtering out "-f" and appending "-o nonempty".
    int new_argc = 0;
    char **new_argv = malloc(sizeof(char *) * (argc + 3));
    if (!new_argv) {
        perror("malloc");
        exit(1);
    }
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0)
            continue;
        new_argv[new_argc++] = argv[i];
    }
    new_argv[new_argc++] = "-o";
    new_argv[new_argc++] = "nonempty";
    new_argv[new_argc] = NULL;

    // Load the file system from disk (or create a new one if no disk image exists).
    fs_load();

    // Initialize FUSE operations.
    nufs_init_ops(&nufs_ops);

    int ret = fuse_main(new_argc, new_argv, &nufs_ops, NULL);
    free(new_argv);
    return ret;
}
