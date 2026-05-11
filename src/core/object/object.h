#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

typedef enum {
    OBJ_BLOB,
    OBJ_TREE,
    OBJ_COMMIT,
    OBJ_DELTA
} obj_type;

/**
 * Write a buffer to the object store.
 * If base_sha256 is provided, it will attempt to store a delta.
 */
char *write_object_ext(const void *buf, size_t len, obj_type type, const char *base_sha256);

// Legacy wrapper
char *write_object(const void *buf, size_t len, obj_type type);

/**
 * Read an object from the object store.
 * Returns the decompressed content of the object.
 */
void *read_object(const char *sha256, size_t *len, obj_type *type);

#endif // OBJECT_H
