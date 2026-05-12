#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_config_path() {
    char *home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    if (!home) return NULL;
    static char path[512];
    snprintf(path, sizeof(path), "%s/.citconfig/config", home);
    return path;
}

void normalize_path(char *path) {
    if (!path || !*path) return;

    // 1. Replace backslashes with forward slashes
    for (char *p = path; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    // 2. Resolve ./ and ../ and double slashes
    char *copy = strdup(path);
    if (!copy) return;

    char *src = copy;
    char *parts[128];
    int part_count = 0;

    // Handle leading slash if present
    int absolute = (copy[0] == '/');
    if (absolute) src++;

    char *token = strtok(src, "/");
    while (token) {
        if (strcmp(token, ".") == 0) {
            // Ignore .
        } else if (strcmp(token, "..") == 0) {
            if (part_count > 0) {
                part_count--;
            }
        } else {
            if (part_count < 128) {
                parts[part_count++] = token;
            }
        }
        token = strtok(NULL, "/");
    }

    // Reconstruct into original buffer
    char *dst = path;
    if (absolute) *dst++ = '/';
    for (int i = 0; i < part_count; i++) {
        size_t len = strlen(parts[i]);
        memcpy(dst, parts[i], len);
        dst += len;
        if (i < part_count - 1) *dst++ = '/';
    }
    if (dst == path && !absolute) {
        *dst++ = '.';
    }
    *dst = '\0';
    free(copy);
}

char *find_cit_root(char *buffer, size_t size) {
    char current[1024];
    if (!getcwd(current, sizeof(current))) return NULL;

    while (1) {
        char cit_path[2048];
        snprintf(cit_path, sizeof(cit_path), "%s/.cit", current);
        if (dir_exists(cit_path)) {
            strncpy(buffer, current, size - 1);
            buffer[size - 1] = '\0';
            return buffer;
        }

        // Move to parent directory
        char *last_slash = strrchr(current, '/');
#ifdef _WIN32
        if (!last_slash) last_slash = strrchr(current, '\\');
#endif

        if (!last_slash || last_slash == current) {
            // Root reached
            if (last_slash == current) {
                // Check root itself if it hasn't been checked
                snprintf(cit_path, sizeof(cit_path), "/.cit");
                if (dir_exists(cit_path)) {
                    strncpy(buffer, "/", size - 1);
                    buffer[size - 1] = '\0';
                    return buffer;
                }
            }
            break;
        }

        *last_slash = '\0';
        if (strlen(current) == 0) break;
    }

    return NULL;
}
