#ifndef UTILS_H
#define UTILS_H

#include "portability.h"
#include <stddef.h>

int mkdir_p(const char *path);
int copy_file(const char *src, const char *dst);
char *file_to_string(const char *path);
int write_string_to_file(const char *path, const char *str);
int dir_exists(const char *path);
int is_file(const char *path);
int is_valid_email(const char *email);
char *get_config_path();

#endif // UTILS_H
