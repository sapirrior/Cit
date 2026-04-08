#ifndef INDEX_H
#define INDEX_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    char path[1024];
    uint8_t sha256[32];
    uint32_t size;
    uint32_t mtime;
} IndexEntry;

typedef struct {
    IndexEntry *entries;
    uint32_t count;
} Index;

Index *read_index();
int write_index(Index *index);
void free_index(Index *index);
int add_to_index(Index *index, const char *path, const uint8_t sha256[32], uint32_t size);

#endif // INDEX_H
