#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int add_file(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *buf = malloc(size);
    if (!buf) {
        fclose(f);
        return -1;
    }
    fread(buf, 1, size, f);
    fclose(f);

    char *sha256_hex = write_object(buf, size, OBJ_BLOB);
    if (!sha256_hex) {
        free(buf);
        return -1;
    }

    uint8_t sha256_bytes[32];
    for (int i = 0; i < 32; i++) {
        sscanf(sha256_hex + (i * 2), "%02hhx", &sha256_bytes[i]);
    }

    add_to_index(index, path, sha256_bytes, (uint32_t)size);
    
    free(buf);
    free(sha256_hex);
    return 0;
}

static int add_recursive(Index *index, const char *path) {
    if (strstr(path, ".cit")) return 0;

    if (is_file(path)) {
        return add_file(index, path);
    } else if (dir_exists(path)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char subpath[1024];
            if (strcmp(path, ".") == 0) {
                snprintf(subpath, sizeof(subpath), "%s", entry->d_name);
            } else {
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            }
            add_recursive(index, subpath);
        }
        closedir(dir);
    }
    return 0;
}

int cmd_add(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Usage: cit add <path>\n");
        return 1;
    }

    Index *index = read_index();
    for (int i = 0; i < argc; i++) {
        add_recursive(index, argv[i]);
    }
    write_index(index);
    free_index(index);
    return 0;
}
