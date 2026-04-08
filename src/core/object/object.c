#include "object.h"
#include "sha256.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static void hash_to_hex(const uint8_t hash[32], char hex[65]) {
    for (int i = 0; i < 32; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[64] = 0;
}

char *write_object(const void *buf, size_t len, obj_type type) {
    const char *type_str;
    switch (type) {
        case OBJ_BLOB: type_str = "blob"; break;
        case OBJ_TREE: type_str = "tree"; break;
        case OBJ_COMMIT: type_str = "commit"; break;
        default: return NULL;
    }

    // Git-style header: "type size\0"
    char header[64];
    int header_len = sprintf(header, "%s %zu", type_str, len) + 1;

    // Combined buffer for hashing and compression
    size_t total_len = header_len + len;
    uint8_t *combined = malloc(total_len);
    if (!combined) return NULL;
    memcpy(combined, header, header_len);
    memcpy(combined + header_len, buf, len);

    // Calculate SHA-256
    SHA256_CTX ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, combined, total_len);
    sha256_final(&ctx, hash);

    char *hex = malloc(65);
    if (!hex) {
        free(combined);
        return NULL;
    }
    hash_to_hex(hash, hex);

    // Path setup
    char dir[256];
    snprintf(dir, sizeof(dir), ".cit/objects/%.2s", hex);
    mkdir_p(dir);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, hex + 2);

    // Compress using compress() for simplicity
    uLongf dest_len = compressBound(total_len);
    void *dest = malloc(dest_len);
    if (!dest) {
        free(combined);
        free(hex);
        return NULL;
    }
    if (compress(dest, &dest_len, combined, total_len) != Z_OK) {
        free(combined);
        free(dest);
        free(hex);
        return NULL;
    }
    free(combined);

    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite(dest, 1, dest_len, f);
        fclose(f);
    }
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

    // Dynamic decompression using inflate
    uLongf dest_cap = 1024 * 1024; // Start with 1MB
    void *dest = malloc(dest_cap);
    if (!dest) {
        free(compressed);
        return NULL;
    }

    z_stream strm = {0};
    strm.next_in = (Bytef *)compressed;
    strm.avail_in = (uInt)compressed_len;
    strm.next_out = (Bytef *)dest;
    strm.avail_out = (uInt)dest_cap;

    if (inflateInit(&strm) != Z_OK) {
        free(compressed);
        free(dest);
        return NULL;
    }

    int ret;
    while ((ret = inflate(&strm, Z_NO_FLUSH)) == Z_BUF_ERROR || ret == Z_OK) {
        if (strm.avail_out == 0) {
            dest_cap *= 2;
            void *new_dest = realloc(dest, dest_cap);
            if (!new_dest) {
                inflateEnd(&strm);
                free(compressed);
                free(dest);
                return NULL;
            }
            dest = new_dest;
            strm.next_out = (Bytef *)dest + strm.total_out;
            strm.avail_out = (uInt)(dest_cap - strm.total_out);
        } else if (ret == Z_OK && strm.avail_in == 0) {
            break; // Finished
        }
    }

    if (ret != Z_STREAM_END && ret != Z_OK) {
        inflateEnd(&strm);
        free(compressed);
        free(dest);
        return NULL;
    }

    size_t total_out = strm.total_out;
    inflateEnd(&strm);
    free(compressed);

    char *header = (char *)dest;
    size_t header_len = 0;
    // Find the null terminator of the header
    for (size_t i = 0; i < total_out; i++) {
        if (header[i] == '\0') {
            header_len = i + 1;
            break;
        }
    }
    
    if (header_len == 0) {
        free(dest);
        return NULL;
    }

    if (strncmp(header, "blob ", 5) == 0) *type = OBJ_BLOB;
    else if (strncmp(header, "tree ", 5) == 0) *type = OBJ_TREE;
    else if (strncmp(header, "commit ", 7) == 0) *type = OBJ_COMMIT;
    else {
        free(dest);
        return NULL;
    }

    size_t data_len = total_out - header_len;
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
