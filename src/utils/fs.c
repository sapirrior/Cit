#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
    int status = 0;
    while (!feof(fsrc) && !ferror(fsrc)) {
        n = fread(buf, 1, sizeof(buf), fsrc);
        if (n > 0) {
            if (fwrite(buf, 1, n, fdst) != n) {
                status = -1;
                break;
            }
        }
    }
    if (ferror(fsrc)) status = -1;
    fclose(fsrc);
    fclose(fdst);
    return status;
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

int remove_directory_recursive(const char *path) {
    DIR *d = opendir(path);
    size_t path_len = strlen(path);
    int r = -1;

    if (d) {
        struct dirent *p;
        r = 0;
        while (!r && (p = readdir(d))) {
            int r2 = -1;
            char *buf;
            size_t len;

            /* Skip the names "." and ".." as we don't want to recurse on them. */
            if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                continue;

            len = path_len + strlen(p->d_name) + 2; 
            buf = malloc(len);

            if (buf) {
                struct stat statbuf;

                snprintf(buf, len, "%s/%s", path, p->d_name);
                if (!stat(buf, &statbuf)) {
                    if (S_ISDIR(statbuf.st_mode))
                        r2 = remove_directory_recursive(buf);
                    else
                        r2 = unlink(buf);
                }
                free(buf);
            }
            r = r2;
        }
        closedir(d);
    }

    if (!r)
        r = rmdir(path);

    return r;
}
