#ifndef DIFF_H
#define DIFF_H

#include <stddef.h>

typedef struct {
    char **lines;
    int count;
} FileContent;

FileContent read_file_lines(const char *path);
FileContent read_string_lines(const char *str);
void free_file_content(FileContent fc);

void diff_files(FileContent a, FileContent b, const char *label_a, const char *label_b);

#endif // DIFF_H
