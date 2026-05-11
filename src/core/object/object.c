#include "object.h"
#include "delta.h"
#include "sha256.h"
#include "utils.h"
#include "cit_compress.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void hash_to_hex(const uint8_t hash[32], char hex[65]) {
    for (int i = 0; i < 32; i++) {
        snprintf(hex + (i * 2), 3, "%02x", hash[i]);
    }
    hex[64] = 0;
}

char *write_object(const void *buf, size_t len, obj_type type) {
    return write_object_ext(buf, len, type, NULL);
}

char *write_object_ext(const void *buf, size_t len, obj_type type, const char *base_sha256) {
    const char *type_str;
    switch (type) {
        case OBJ_BLOB: type_str = "blob"; break;
        case OBJ_TREE: type_str = "tree"; break;
        case OBJ_COMMIT: type_str = "commit"; break;
        default: return NULL;
    }

    // 1. Calculate Content SHA (Always use the full content for the SHA)
    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, len) + 1;
    size_t full_len = header_len + len;
    uint8_t *full_buf = malloc(full_len);
    if (!full_buf) return NULL;
    memcpy(full_buf, header, header_len);
    memcpy(full_buf + header_len, buf, len);

    SHA256_CTX ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, full_buf, full_len);
    sha256_final(&ctx, hash);

    char *hex = malloc(65);
    if (!hex) {
        free(full_buf);
        return NULL;
    }
    hash_to_hex(hash, hex);

    // 2. Check if object already exists
    char dir[256];
    snprintf(dir, sizeof(dir), ".cit/objects/%.2s", hex);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, hex + 2);
    
    if (is_file(path)) {
        free(full_buf);
        return hex;
    }

    // 3. Decide: Store as Delta or Full?
    void *storage_buf = full_buf;
    size_t storage_len = full_len;
    int is_delta = 0;

    if (base_sha256 && type == OBJ_BLOB) {
        size_t base_len;
        obj_type base_type;
        void *base_data = read_object(base_sha256, &base_len, &base_type);
        if (base_data) {
            size_t d_len;
            void *d_data = diff_delta(base_data, base_len, buf, len, &d_len);
            
            // Header for delta: "delta <target_len> <base_sha>\0"
            char d_header[128];
            int d_header_len = snprintf(d_header, sizeof(d_header), "delta %zu %s", len, base_sha256) + 1;
            
            if (d_header_len + d_len < full_len / 2) {
                storage_len = d_header_len + d_len;
                storage_buf = malloc(storage_len);
                if (storage_buf) {
                    memcpy(storage_buf, d_header, d_header_len);
                    memcpy((uint8_t *)storage_buf + d_header_len, d_data, d_len);
                    is_delta = 1;
                } else {
                    // Fallback to full storage if delta allocation fails
                    storage_buf = full_buf;
                    storage_len = full_len;
                }
            }
            free(d_data);
            free(base_data);
        }
    }

    // 4. Compress and Save
    mkdir_p(dir);
    size_t dest_cap = cit_compress_bound(storage_len);
    void *dest = malloc(dest_cap);
    if (!dest) {
        if (is_delta) free(storage_buf);
        free(full_buf);
        free(hex);
        return NULL;
    }
    size_t dest_len = 0;
    cit_compress(storage_buf, storage_len, dest, &dest_len);
    
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(dest, 1, dest_len, f);
        fclose(f);
    }

    if (is_delta) free(storage_buf);
    free(full_buf);
    free(dest);
    return hex;
}

void *read_object(const char *hex, size_t *len, obj_type *type) {
    char path[512];
    snprintf(path, sizeof(path), ".cit/objects/%.2s/%s", hex, hex + 2);

    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long compressed_len = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *compressed = malloc(compressed_len);
    if (!compressed) {
        fclose(f);
        return NULL;
    }
    if (fread(compressed, 1, compressed_len, f) != (size_t)compressed_len) {
        free(compressed);
        fclose(f);
        return NULL;
    }
    fclose(f);

    uint64_t original_size = *(uint64_t *)((uint8_t *)compressed + 4);
    void *dest = malloc(original_size + 1);
    if (!dest) {
        free(compressed);
        return NULL;
    }
    cit_decompress(compressed, compressed_len, dest, original_size);
    free(compressed);

    char *header = (char *)dest;
    size_t header_len = 0;
    for (size_t i = 0; i < original_size; i++) {
        if (header[i] == '\0') {
            header_len = i + 1;
            break;
        }
    }

    if (strncmp(header, "delta ", 6) == 0) {
        size_t target_len;
        char base_sha[65];
        sscanf(header + 6, "%zu %64s", &target_len, base_sha);
        
        size_t base_len;
        obj_type base_type;
        void *base_data = read_object(base_sha, &base_len, &base_type);
        if (!base_data) {
            free(dest);
            return NULL;
        }
        
        void *target_data = patch_delta(base_data, base_len, (uint8_t *)dest + header_len, original_size - header_len, target_len);
        
        free(base_data);
        free(dest);
        *len = target_len;
        *type = OBJ_BLOB; 
        return target_data;
    }

    if (strncmp(header, "blob ", 5) == 0) *type = OBJ_BLOB;
    else if (strncmp(header, "tree ", 5) == 0) *type = OBJ_TREE;
    else if (strncmp(header, "commit ", 7) == 0) *type = OBJ_COMMIT;
    else {
        free(dest);
        return NULL;
    }
    
    size_t data_len = original_size - header_len;
    *len = data_len;
    void *result = malloc(data_len + 1);
    if (!result) {
        free(dest);
        return NULL;
    }
    memcpy(result, (char *)dest + header_len, data_len);
    ((char *)result)[data_len] = 0;
    free(dest);
    return result;
}
