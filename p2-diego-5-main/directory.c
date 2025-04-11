#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "directory.h"
#include "inode.h"
#include "storage.h"
#include "blocks.h"
#include "bitmap.h"
#include "slist.h"

#define BLOCK_SIZE 4096

void directory_init() {
    // Optional: any global directory initialization code.
}

/* Look up an entry in the directory represented by inode *di.
 * Returns the inode number if found, or -ENOENT if not found.
 */
int directory_lookup(inode_t *di, const char *name) {
    if (!di) return -ENOENT;
    int nentries = di->size / sizeof(fs_dirent_t);
    if (nentries == 0)
        return -ENOENT;
    fs_dirent_t *entries = malloc(di->size);
    if (!entries) return -1;
    if (storage_read_block(di->block, (char*)entries, 0, di->size) != di->size) {
        free(entries);
        return -1;
    }
    for (int i = 0; i < nentries; i++) {
        if (entries[i].inum != -1 && strcmp(entries[i].name, name) == 0) {
            int found_inum = entries[i].inum;
            free(entries);
            return found_inum;
        }
    }
    free(entries);
    return -ENOENT;
}

/* Add a new entry (name and inode number) to a directory */
int directory_put(inode_t *di, const char *name, int inum) {
    if (!di) return -ENOENT;
    int nentries = di->size / sizeof(fs_dirent_t);
    fs_dirent_t *entries = malloc(BLOCK_SIZE);
    if (!entries) return -1;
    if (di->block < 0) {
        int blk = alloc_block();
        if (blk < 0) {
            free(entries);
            return -ENOSPC;
        }
        di->block = blk;
        memset(entries, 0, BLOCK_SIZE);
        nentries = 0;
    } else {
        if (storage_read_block(di->block, (char*)entries, 0, BLOCK_SIZE) < 0) {
            free(entries);
            return -1;
        }
    }
    int slot = -1;
    for (int i = 0; i < nentries; i++) {
        if (entries[i].inum == -1) {
            slot = i;
            break;
        }
    }
    if (slot == -1) { 
        slot = nentries;
        nentries++;
    }
    strncpy(entries[slot].name, name, DIR_NAME_LENGTH - 1);
    entries[slot].name[DIR_NAME_LENGTH - 1] = '\0';
    entries[slot].inum = inum;
    if (storage_write_block(di->block, (char*)entries, 0, BLOCK_SIZE) != BLOCK_SIZE) {
        free(entries);
        return -1;
    }
    di->size = nentries * sizeof(fs_dirent_t);
    free(entries);
    return 0;
}

/* Delete an entry from the directory */
int directory_delete(inode_t *di, const char *name) {
    if (!di) return -ENOENT;
    int nentries = di->size / sizeof(fs_dirent_t);
    fs_dirent_t *entries = malloc(di->size);
    if (!entries) return -1;
    if (storage_read_block(di->block, (char*)entries, 0, di->size) != di->size) {
        free(entries);
        return -1;
    }
    int found = 0;
    for (int i = 0; i < nentries; i++) {
        if (entries[i].inum != -1 && strcmp(entries[i].name, name) == 0) {
            entries[i].inum = -1;
            memset(entries[i].name, 0, DIR_NAME_LENGTH);
            found = 1;
            break;
        }
    }
    if (!found) {
        free(entries);
        return -ENOENT;
    }
    if (storage_write_block(di->block, (char*)entries, 0, di->size) != di->size) {
        free(entries);
        return -1;
    }
    free(entries);
    return 0;
}

/* Return a singly-linked list of valid directory entries.
 * For each entry, allocate a copy of the fs_dirent_t and store it in the list.
 */
slist_t *directory_list(const char *path) {
    extern inode_t *path_lookup(const char *);  // defined in nufs.c
    inode_t *di = path_lookup(path);
    if (!di) return NULL;
    int nentries = di->size / sizeof(fs_dirent_t);
    if (nentries == 0) return NULL;
    fs_dirent_t *entries = malloc(di->size);
    if (!entries) return NULL;
    if (storage_read_block(di->block, (char*)entries, 0, di->size) != di->size) {
        free(entries);
        return NULL;
    }
    slist_t *list = NULL;
    for (int i = nentries - 1; i >= 0; i--) {
        if (entries[i].inum != -1) {
            fs_dirent_t *entry_copy = malloc(sizeof(fs_dirent_t));
            if (!entry_copy) continue;
            memcpy(entry_copy, &entries[i], sizeof(fs_dirent_t));
            list = slist_cons_ptr(entry_copy, list);
        }
    }
    free(entries);
    return list;
}

/* Print the contents of a directory to stdout */
void print_directory(inode_t *di) {
    if (!di) return;
    int nentries = di->size / sizeof(fs_dirent_t);
    printf("Directory (inode):\n");
    fs_dirent_t *entries = malloc(di->size);
    if (!entries) return;
    if (storage_read_block(di->block, (char*)entries, 0, di->size) != di->size) {
        free(entries);
        return;
    }
    for (int i = 0; i < nentries; i++) {
        if (entries[i].inum != -1) {
            printf("  %s (inum: %d)\n", entries[i].name, entries[i].inum);
        }
    }
    free(entries);
}
