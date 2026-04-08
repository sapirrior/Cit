#include "commands.h"
#include "object.h"
#include "ui.h"
#include "diff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_show(int argc, char *argv[]) {
    if (argc < 2) {
        ui_error("Usage: cit show <commit-sha>");
        return 1;
    }

    char *target_sha = argv[1];
    size_t obj_len;
    obj_type type;
    char *content = read_object(target_sha, &obj_len, &type);

    if (!content) {
        ui_error("Could not read object %s", target_sha);
        return 1;
    }

    if (type != OBJ_COMMIT) {
        ui_error("Object %s is not a commit", target_sha);
        free(content);
        return 1;
    }

    // Parse commit
    char tree_sha[65] = "";
    char parent_sha[65] = "";
    char *message = "";

    char *tree_line = strstr(content, "tree ");
    if (tree_line) {
        strncpy(tree_sha, tree_line + 5, 64);
        tree_sha[64] = 0;
    }

    char *parent_line = strstr(content, "parent ");
    if (parent_line) {
        strncpy(parent_sha, parent_line + 7, 64);
        parent_sha[64] = 0;
    }

    char *msg_ptr = strstr(content, "\n\n");
    if (msg_ptr) message = msg_ptr + 2;

    // Display Header
    printf(COLOR_BOLD COLOR_FG_ATTENTION "commit %s" COLOR_RESET "\n", target_sha);
    printf("Author: Cit User\n"); // TODO: Read from config
    printf("\n    %s\n", message);

    // Diff with parent
    if (strlen(parent_sha) > 0) {
        // TODO: Implement tree comparison for full commit show
        ui_info("\n(Diff with parent %s coming soon...)", parent_sha);
    } else {
        ui_info("\n(Initial commit)");
    }

    free(content);
    return 0;
}
