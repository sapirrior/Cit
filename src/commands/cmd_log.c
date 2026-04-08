#include "commands.h"
#include "object.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_log(int argc, char *argv[]) {
    char current_sha256[65] = "";
    
    char *current_branch = get_current_branch();
    if (current_branch) {
        char ref_path[512];
        snprintf(ref_path, sizeof(ref_path), ".cit/refs/heads/%s", current_branch);
        FILE *fref = fopen(ref_path, "r");
        if (fref) {
            if (!fgets(current_sha256, 65, fref)) current_sha256[0] = 0;
            fclose(fref);
        }
    }

    if (strlen(current_sha256) == 0) {
        ui_info("No commits yet.");
        return 0;
    }

    while (strlen(current_sha256) > 0) {
        size_t obj_len;
        obj_type type;
        char *content = read_object(current_sha256, &obj_len, &type);
        if (!content || type != OBJ_COMMIT) {
            if (content) free(content);
            break;
        }

        printf(COLOR_FG_ATTENTION "commit %s" COLOR_RESET "\n", current_sha256);
        
        char *message = strstr(content, "\n\n");
        if (message) {
            message += 2;
            printf("    %s", message);
        }
        printf("\n");

        char *parent_ptr = strstr(content, "parent ");
        if (parent_ptr) {
            strncpy(current_sha256, parent_ptr + 7, 64);
            current_sha256[64] = 0;
        } else {
            current_sha256[0] = 0;
        }
        
        free(content);
    }

    return 0;
}
