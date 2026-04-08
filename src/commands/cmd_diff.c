#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include "diff.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_diff(int argc, char *argv[]) {
    Index *index = read_index();
    if (!index) return 1;

    int staged = (argc > 1 && strcmp(argv[1], "--staged") == 0);

    if (staged) {
        // Staged diff: Index vs HEAD
        ui_header("Showing staged changes (Index vs HEAD)");
        // TODO: Implement HEAD tree parsing and comparison
        ui_warn("Staged diff is not fully implemented yet.");
    } else {
        // Unstaged diff: Working Tree vs Index
        for (uint32_t i = 0; i < index->count; i++) {
            if (!is_file(index->entries[i].path)) continue;

            // Read from index
            size_t obj_len;
            obj_type type;
            char hex[65];
            for (int j = 0; j < 32; j++) sprintf(hex + (j * 2), "%02x", index->entries[i].sha256[j]);
            hex[64] = 0;

            char *index_content = read_object(hex, &obj_len, &type);
            if (!index_content) continue;

            FileContent a = read_string_lines(index_content);
            FileContent b = read_file_lines(index->entries[i].path);

            // Check if different before doing full diff
            // Actually diff_files will handle it, but we only want to show if changed
            struct stat st;
            stat(index->entries[i].path, &st);
            if ((uint32_t)st.st_mtime != index->entries[i].mtime || (uint32_t)st.st_size != index->entries[i].size) {
                printf(COLOR_FG_ACCENT "diff --cit a/%s b/%s" COLOR_RESET "\n", index->entries[i].path, index->entries[i].path);
                diff_files(a, b, index->entries[i].path, index->entries[i].path);
                printf("\n");
            }

            free(index_content);
            free_file_content(a);
            free_file_content(b);
        }
    }

    free_index(index);
    return 0;
}
