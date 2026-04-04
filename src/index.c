#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

Index *read_index() {
    Index *index = malloc(sizeof(Index));
    if (!index) return NULL;
    index->count = 0;
    index->entries = NULL;

    FILE *f = fopen(".cit/index", "rb");
    if (!f) return index;

    if (fread(&index->count, sizeof(uint32_t), 1, f) == 1 && index->count > 0) {
        index->entries = malloc(sizeof(IndexEntry) * index->count);
        if (index->entries) {
            fread(index->entries, sizeof(IndexEntry), index->count, f);
        } else {
            index->count = 0;
        }
    }
    fclose(f);
    return index;
}

int write_index(Index *index) {
    FILE *f = fopen(".cit/index", "wb");
    if (!f) return -1;

    fwrite(&index->count, sizeof(uint32_t), 1, f);
    if (index->count > 0) {
        fwrite(index->entries, sizeof(IndexEntry), index->count, f);
    }
    fclose(f);
    return 0;
}

void free_index(Index *index) {
    if (index->entries) free(index->entries);
    free(index);
}

int add_to_index(Index *index, const char *path, const uint8_t sha256[32], uint32_t size) {
    // Check if entry already exists
    for (uint32_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            memcpy(index->entries[i].sha256, sha256, 32);
            index->entries[i].size = size;
            // Update mtime
            struct stat st;
            stat(path, &st);
            index->entries[i].mtime = st.st_mtime;
            return 0;
        }
    }

    // Add new entry
    index->count++;
    IndexEntry *new_entries = realloc(index->entries, sizeof(IndexEntry) * index->count);
    if (!new_entries) {
        fprintf(stderr, "Error: Memory allocation failed during index update.\n");
        index->count--;
        return -1;
    }
    index->entries = new_entries;
    
    strcpy(index->entries[index->count - 1].path, path);
    memcpy(index->entries[index->count - 1].sha256, sha256, 32);
    index->entries[index->count - 1].size = size;
    
    struct stat st;
    stat(path, &st);
    index->entries[index->count - 1].mtime = st.st_mtime;
    
    return 0;
}
