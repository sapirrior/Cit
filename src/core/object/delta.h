#ifndef DELTA_H
#define DELTA_H

#include <stddef.h>
#include <stdint.h>

/**
 * Generates a delta from base to target.
 * Returns a newly allocated buffer containing the delta.
 * *delta_len will contain the length of the delta buffer.
 */
void *diff_delta(const void *base, size_t base_len,
                const void *target, size_t target_len,
                size_t *delta_len);

/**
 * Applies a delta to a base buffer to reconstruct the target.
 * Returns a newly allocated buffer containing the target.
 */
void *patch_delta(const void *base, size_t base_len,
                 const void *delta, size_t delta_len,
                 size_t target_len);

#endif // DELTA_H
