#include "delta.h"
#include <stdlib.h>
#include <string.h>

#define MIN_MATCH 4

void *diff_delta(const void *base, size_t base_len,
                const void *target, size_t target_len,
                size_t *delta_len) {
    // Basic delta: [CMD] [LEN] [DATA/OFFSET]
    // CMD 0: Copy (Offset, Len)
    // CMD 1: Insert (Len, Data)
    
    uint8_t *delta = malloc(target_len * 2); // Over-allocation
    if (!delta) return NULL;
    size_t d_pos = 0;
    size_t t_pos = 0;

    while (t_pos < target_len) {
        size_t best_len = 0;
        size_t best_off = 0;

        // Simple search for match in base
        if (base_len >= MIN_MATCH) {
            for (size_t b_pos = 0; b_pos <= base_len - MIN_MATCH; b_pos++) {
                size_t match_len = 0;
                while (b_pos + match_len < base_len && 
                       t_pos + match_len < target_len &&
                       ((uint8_t *)base)[b_pos + match_len] == ((uint8_t *)target)[t_pos + match_len]) {
                    match_len++;
                }
                if (match_len > best_len) {
                    best_len = match_len;
                    best_off = b_pos;
                }
            }
        }

        if (best_len >= MIN_MATCH) {
            // Copy command
            delta[d_pos++] = 0; 
            memcpy(delta + d_pos, &best_off, sizeof(size_t));
            d_pos += sizeof(size_t);
            memcpy(delta + d_pos, &best_len, sizeof(size_t));
            d_pos += sizeof(size_t);
            t_pos += best_len;
        } else {
            // Insert command
            size_t ins_start = t_pos;
            while (t_pos < target_len) {
                // Peek ahead to see if we find a match
                int found_match = 0;
                if (target_len - t_pos >= MIN_MATCH) {
                    for (size_t b_pos = 0; b_pos <= base_len - MIN_MATCH; b_pos++) {
                        if (memcmp((uint8_t *)base + b_pos, (uint8_t *)target + t_pos, MIN_MATCH) == 0) {
                            found_match = 1;
                            break;
                        }
                    }
                }
                if (found_match) break;
                t_pos++;
            }
            size_t ins_len = t_pos - ins_start;
            delta[d_pos++] = 1;
            memcpy(delta + d_pos, &ins_len, sizeof(size_t));
            d_pos += sizeof(size_t);
            memcpy(delta + d_pos, (uint8_t *)target + ins_start, ins_len);
            d_pos += ins_len;
        }
    }

    *delta_len = d_pos;
    void *new_delta = realloc(delta, d_pos);
    return new_delta ? new_delta : delta;
}

void *patch_delta(const void *base, size_t base_len,
                 const void *delta, size_t delta_len,
                 size_t target_len) {
    uint8_t *target = malloc(target_len + 1);
    if (!target) return NULL;
    size_t t_pos = 0;
    size_t d_pos = 0;
    uint8_t *d = (uint8_t *)delta;

    while (d_pos < delta_len) {
        uint8_t cmd = d[d_pos++];
        if (cmd == 0) {
            // Copy
            size_t off, len;
            memcpy(&off, d + d_pos, sizeof(size_t));
            d_pos += sizeof(size_t);
            memcpy(&len, d + d_pos, sizeof(size_t));
            d_pos += sizeof(size_t);
            memcpy(target + t_pos, (uint8_t *)base + off, len);
            t_pos += len;
        } else if (cmd == 1) {
            // Insert
            size_t len;
            memcpy(&len, d + d_pos, sizeof(size_t));
            d_pos += sizeof(size_t);
            memcpy(target + t_pos, d + d_pos, len);
            d_pos += len;
            t_pos += len;
        }
    }
    target[target_len] = 0;
    return target;
}
