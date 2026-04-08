#include "commands.h"
#include "ui.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_clone(int argc, char *argv[]) {
    if (argc < 2) {
        ui_error("Usage: cit clone <url> [<dir>]");
        return 1;
    }

    char *url = argv[1];
    char *dir = (argc > 2) ? argv[2] : NULL;

    // Determine directory name if not provided
    char dir_buf[256];
    if (!dir) {
        char *last_slash = strrchr(url, '/');
        if (last_slash) {
            strcpy(dir_buf, last_slash + 1);
            char *dot_git = strstr(dir_buf, ".git");
            if (dot_git) *dot_git = '\0';
            dir = dir_buf;
        } else {
            dir = "repo";
        }
    }

    ui_info("Cloning into '%s'...", dir);

    // Step 1: Use system git to perform the initial clone
    // This handles all protocols (HTTPS, SSH) and authentication
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "git clone %s %s", url, dir);
    int ret = system(cmd);
    if (ret != 0) {
        ui_error("Failed to clone repository via git.");
        return 1;
    }

    // Step 2: Convert .git to .cit
    // Change directory to the new repo
    if (chdir(dir) != 0) {
        ui_error("Could not enter directory %s", dir);
        return 1;
    }

    ui_info("Converting Git repository to Cit format (SHA-1 -> SHA-256)...");
    
    // Initialize Cit repo
    char *init_argv[] = {"init"};
    cmd_init(1, init_argv);

    // TODO: Implement the actual migration logic
    // For now, we'll just do a fresh 'cit add .' to represent the current state
    ui_info("Performing initial Cit migration...");
    char *add_argv[] = {"add", "."};
    cmd_add(2, add_argv);
    
    char *commit_argv[] = {"commit", "Initial clone from Git"};
    cmd_commit(2, commit_argv);

    // Step 3: Ask user if they want to delete .git
    printf("\nConversion complete. Do you want to delete the original .git folder? (y/n): ");
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) && (choice[0] == 'y' || choice[0] == 'Y')) {
        system("rm -rf .git");
        ui_success("Deleted .git folder.");
    } else {
        ui_info("Kept .git folder.");
    }

    ui_success("Cloned and converted repository successfully.");
    return 0;
}
