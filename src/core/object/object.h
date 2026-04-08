#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>

typedef enum {
    OBJ_BLOB,
    OBJ_TREE,
    OBJ_COMMIT
} obj_type;

/**
 * Write a buffer to the object store as a blob.
 * Returns the SHA-256 hash of the object (as a hex string).
 */
char *write_object(const void *buf, size_t len, obj_type type);

/**
 * Read an object from the object store.
 * Returns the decompressed content of the object.
 */
void *read_object(const char *sha256, size_t *len, obj_type *type);

#endif // OBJECT_H
