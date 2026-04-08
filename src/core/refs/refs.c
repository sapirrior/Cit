#include "refs.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *get_current_branch() {
    FILE *fhead = fopen(".cit/HEAD", "r");
    if (!fhead) return NULL;
    static char head_content[256];
    if (fgets(head_content, sizeof(head_content), fhead)) {
        if (strncmp(head_content, "ref: refs/heads/", 16) == 0) {
            char *branch = head_content + 16;
            // Trim whitespace
            char *p = branch;
            while (*p && *p != ' ' && *p != '\n' && *p != '\r') p++;
            *p = 0;
            fclose(fhead);
            return branch;
        }
    }
    fclose(fhead);
    return NULL;
}

int update_ref(const char *ref_path, const char *sha256) {
    char full_ref_path[512];
    snprintf(full_ref_path, sizeof(full_ref_path), ".cit/%s", ref_path);
    return write_string_to_file(full_ref_path, sha256);
}
