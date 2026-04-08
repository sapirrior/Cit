#include "diff.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FileContent read_file_lines(const char *path) {
    FILE *f = fopen(path, "r");
    FileContent fc = {NULL, 0};
    if (!f) return fc;

    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        fc.lines = realloc(fc.lines, sizeof(char *) * (fc.count + 1));
        fc.lines[fc.count++] = strdup(line);
    }
    fclose(f);
    return fc;
}

FileContent read_string_lines(const char *str) {
    FileContent fc = {NULL, 0};
    if (!str) return fc;

    char *s = strdup(str);
    char *p = strtok(s, "\n");
    while (p) {
        fc.lines = realloc(fc.lines, sizeof(char *) * (fc.count + 1));
        // Add newline back if it was stripped
        char *line = malloc(strlen(p) + 2);
        sprintf(line, "%s\n", p);
        fc.lines[fc.count++] = line;
        p = strtok(NULL, "\n");
    }
    free(s);
    return fc;
}

void free_file_content(FileContent fc) {
    for (int i = 0; i < fc.count; i++) free(fc.lines[i]);
    free(fc.lines);
}

static void backtrack(int **trace, FileContent a, FileContent b, int d, int x, int y) {
    if (d == 0) return;
    int k = x - y;
    int MAX = a.count + b.count;
    int prev_k;

    if (k == -d || (k != d && trace[d-1][MAX + k - 1] < trace[d-1][MAX + k + 1])) {
        prev_k = k + 1;
    } else {
        prev_k = k - 1;
    }

    int prev_x = trace[d-1][MAX + prev_k];
    int prev_y = prev_x - prev_k;

    while (x > prev_x && y > prev_y) {
        x--; y--;
    }

    backtrack(trace, a, b, d - 1, prev_x, prev_y);

    if (x > prev_x) {
        printf(COLOR_FG_DANGER "- %s" COLOR_RESET, a.lines[prev_x]);
    } else {
        printf(COLOR_FG_SUCCESS "+ %s" COLOR_RESET, b.lines[prev_y]);
    }
}

void diff_files(FileContent a, FileContent b, const char *label_a, const char *label_b) {
    int N = a.count;
    int M = b.count;
    int MAX = N + M;
    if (MAX == 0) return;

    int *v = calloc(2 * MAX + 1, sizeof(int));
    int **trace = malloc(sizeof(int *) * (MAX + 1));
    int d, k;

    printf(COLOR_BOLD "--- %s\n+++ %s" COLOR_RESET "\n", label_a, label_b);

    for (d = 0; d <= MAX; d++) {
        trace[d] = malloc(sizeof(int) * (2 * MAX + 1));
        for (k = -d; k <= d; k += 2) {
            int x, y;
            if (k == -d || (k != d && v[MAX + k - 1] < v[MAX + k + 1])) {
                x = v[MAX + k + 1];
            } else {
                x = v[MAX + k - 1] + 1;
            }
            y = x - k;

            while (x < N && y < M && strcmp(a.lines[x], b.lines[y]) == 0) {
                x++; y++;
            }

            v[MAX + k] = x;
            trace[d][MAX + k] = x;

            if (x >= N && y >= M) {
                backtrack(trace, a, b, d, N, M);
                goto done;
            }
        }
    }

done:
    for (int i = 0; i <= d; i++) free(trace[i]);
    free(trace);
    free(v);
}
