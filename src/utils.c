#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

int mkdir_p(const char *path) {
    char tmp[1024];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/' || tmp[len - 1] == '\\')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++) {
        if (*p == '/' || *p == '\\') {
            char sep = *p;
            *p = 0;
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = sep;
        }
    }
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int dir_exists(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && (st.st_mode & S_IFDIR));
}

int is_file(const char *path) {
    struct stat st;
    return (stat(path, &st) == 0 && (st.st_mode & S_IFREG));
}

int copy_file(const char *src, const char *dst) {
    FILE *fsrc = fopen(src, "rb");
    if (!fsrc) return -1;
    FILE *fdst = fopen(dst, "wb");
    if (!fdst) {
        fclose(fsrc);
        return -1;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
        if (fwrite(buf, 1, n, fdst) != n) {
            fclose(fsrc);
            fclose(fdst);
            return -1;
        }
    }
    fclose(fsrc);
    fclose(fdst);
    return 0;
}

char *file_to_string(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    long len = ftell(f);
    if (len < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    char *s = malloc(len + 1);
    if (!s) {
        fclose(f);
        return NULL;
    }
    size_t read_len = fread(s, 1, len, f);
    if (read_len != (size_t)len) {
        free(s);
        fclose(f);
        return NULL;
    }
    s[len] = 0;
    fclose(f);
    return s;
}

int write_string_to_file(const char *path, const char *str) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t len = strlen(str);
    size_t written = fwrite(str, 1, len, f);
    fclose(f);
    return (written == len) ? 0 : -1;
}

int is_valid_email(const char *email) {
    if (!email || !*email) return 0;

    // Rule: Cannot start with a digit
    if (isdigit((unsigned char)email[0])) return 0;

    // Rule: Must start with an alphanumeric character
    if (!isalnum((unsigned char)email[0])) return 0;

    const char *at = strchr(email, '@');
    if (!at) return 0; // Must have '@'
    if (strchr(at + 1, '@')) return 0; // Must have ONLY one '@'

    // Local part (before @)
    if (at == email) return 0; // Cannot be empty

    for (const char *p = email; p < at; p++) {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '_' && *p != '-' && *p != '+') {
            return 0; // Invalid character in local part
        }
    }

    // Domain part (after @)
    const char *domain = at + 1;
    if (!*domain) return 0; // Domain cannot be empty

    const char *dot = strrchr(domain, '.');
    if (!dot || dot == domain || !*(dot + 1)) return 0; // Must have a '.' and it can't be first or last

    // TLD (after last dot) should be at least 2 chars
    if (strlen(dot + 1) < 2) return 0;

    for (const char *p = domain; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '.' && *p != '-') {
            return 0; // Invalid character in domain
        }
    }

    return 1;
}

char *get_config_path() {
    char *home = getenv("HOME");
    if (!home) home = getenv("USERPROFILE");
    if (!home) return NULL;
    static char path[512];
    snprintf(path, sizeof(path), "%s/.citconfig/config", home);
    return path;
}
