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
            strncpy(dir_buf, last_slash + 1, sizeof(dir_buf) - 1);
            dir_buf[sizeof(dir_buf) - 1] = '\0';
            char *dot_git = strstr(dir_buf, ".git");
            if (dot_git) *dot_git = '\0';
            dir = dir_buf;
        } else {
            dir = "repo";
        }
    }

    if (dir_exists(dir)) {
        ui_error("Directory '%s' already exists.", dir);
        return 1;
    }

    ui_info("Cloning into '%s'...", dir);

    // Step 1: Use git clone safely
    char *git_argv[] = {"git", "clone", url, dir, NULL};
    int ret = safe_execute("git", git_argv);

    if (ret != 0) {
        ui_error("Failed to clone repository via git.");
        // Cleanup partial directory if it was created
        if (dir_exists(dir)) {
            remove_directory_recursive(dir);
        }
        return 1;
    }

    // Save current working directory to return later
    char original_cwd[512];
    if (!getcwd(original_cwd, sizeof(original_cwd))) {
        ui_error("Could not get current working directory.");
        return 1;
    }

    // Step 2: Convert .git to .cit
    if (chdir(dir) != 0) {
        ui_error("Could not enter directory %s", dir);
        remove_directory_recursive(dir);
        return 1;
    }

    ui_info("Converting Git repository to Cit format...");

    // Initialize Cit repo
    if (cmd_init(0, NULL) != 0) {
        ui_error("Failed to initialize Cit repository.");
        chdir(original_cwd);
        remove_directory_recursive(dir);
        return 1;
    }

    ui_info("Performing initial Cit migration...");
    char *add_argv[] = {"add", ".", NULL};
    if (cmd_add(2, add_argv) != 0) {
        ui_error("Failed to stage files.");
        chdir(original_cwd);
        remove_directory_recursive(dir);
        return 1;
    }

    char *commit_argv[] = {"commit", "Initial clone from Git", NULL};
    if (cmd_commit(2, commit_argv) != 0) {
        ui_error("Failed to perform initial commit.");
        chdir(original_cwd);
        remove_directory_recursive(dir);
        return 1;
    }

    // Step 3: Ask user if they want to delete .git
    printf("\nConversion complete. Do you want to delete the original .git folder? (y/n): ");
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) && (choice[0] == 'y' || choice[0] == 'Y')) {
        if (remove_directory_recursive(".git") == 0) {
            ui_success("Deleted .git folder.");
        } else {
            ui_error("Failed to delete .git folder.");
        }
    } else {
        ui_info("Kept .git folder.");
    }

    chdir(original_cwd);
    ui_success("Cloned and converted repository successfully.");
    return 0;
}

