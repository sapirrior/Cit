#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include "config.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_commit(int argc, char *argv[]) {
    if (!check_config()) {
        fprintf(stderr, "Error: User configuration (username and email) not found.\n");
        fprintf(stderr, "Use 'cit config -u <name>' and 'cit config -e <email>' to set them.\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: cit commit <message>\n");
        return 1;
    }

    char *message = argv[1];
    Index *index = read_index();
    if (index->count == 0) {
        printf("Nothing to commit (create/copy files and use \"cit add\")\n");
        free_index(index);
        return 0;
    }

    // 1. Create Tree object from Index
    size_t tree_size = 0;
    for (uint32_t i = 0; i < index->count; i++) {
        tree_size += 64 + 1 + strlen(index->entries[i].path) + 1;
    }

    char *tree_buf = malloc(tree_size + 1);
    if (!tree_buf) {
        free_index(index);
        return 1;
    }
    char *p = tree_buf;
    for (uint32_t i = 0; i < index->count; i++) {
        for (int j = 0; j < 32; j++) {
            p += sprintf(p, "%02x", index->entries[i].sha256[j]);
        }
        p += sprintf(p, " %s\n", index->entries[i].path);
    }
    
    char *tree_sha256 = write_object(tree_buf, tree_size, OBJ_TREE);
    free(tree_buf);
    if (!tree_sha256) {
        free_index(index);
        return 1;
    }

    // 2. Get parent commit SHA from HEAD
    char parent_sha256[65] = "";
    char *current_branch = get_current_branch();
    if (current_branch) {
        char ref_path[512];
        snprintf(ref_path, sizeof(ref_path), ".cit/refs/heads/%s", current_branch);
        FILE *fref = fopen(ref_path, "r");
        if (fref) {
            if (!fgets(parent_sha256, 65, fref)) parent_sha256[0] = 0;
            fclose(fref);
        }
    }

    // 3. Create Commit object
    size_t commit_buf_size = 128 + 64 + 64 + strlen(message);
    char *commit_buf = malloc(commit_buf_size);
    if (!commit_buf) {
        free(tree_sha256);
        free_index(index);
        return 1;
    }

    int commit_len;
    if (strlen(parent_sha256) > 0) {
        commit_len = sprintf(commit_buf, "tree %s\nparent %s\n\n%s\n", tree_sha256, parent_sha256, message);
    } else {
        commit_len = sprintf(commit_buf, "tree %s\n\n%s\n", tree_sha256, message);
    }

    char *commit_sha256 = write_object(commit_buf, commit_len, OBJ_COMMIT);
    free(commit_buf);

    if (!commit_sha256) {
        free(tree_sha256);
        free_index(index);
        return 1;
    }

    // 4. Update ref (branch)
    if (current_branch) {
        char ref_path[512];
        snprintf(ref_path, sizeof(ref_path), "refs/heads/%s", current_branch);
        update_ref(ref_path, commit_sha256);
    }

    printf("[" COLOR_FG_ACCENT "%s" COLOR_RESET " " COLOR_FG_ATTENTION "%.7s" COLOR_RESET "] %s\n", current_branch ? current_branch : "detached", commit_sha256, message);

    free(tree_sha256);
    free(commit_sha256);
    free_index(index);
    return 0;
}
