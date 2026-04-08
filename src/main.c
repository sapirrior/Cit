#include <stdio.h>
#include <string.h>
#include "commands.h"

void print_help() {
    printf("Usage: cit <command> [<args>]\n");
    printf("\nAvailable commands:\n");
    printf("  init      Initialize a new Cit repository\n");
    printf("  clone     Clone a repository and convert to Cit\n");
    printf("  add       Add file contents to the staging area\n");
    printf("  commit    Record changes to the repository\n");
    printf("  status    Show the working tree status\n");
    printf("  diff      Show changes between commits, commit and working tree, etc\n");
    printf("  show      Show various types of objects\n");
    printf("  reset     Reset current HEAD to the specified state\n");
    printf("  log       Show commit logs\n");
    printf("  branch    List, create, or delete branches\n");
    printf("  checkout  Switch branches or restore working tree files\n");
    printf("  config    Get and set repository or global options\n");
    printf("\nFlags:\n");
    printf("  -h, --help  Show this help message\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_help();
        return (argc < 2) ? 1 : 0;
    }

    char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        return cmd_init(argc - 1, argv + 1);
    } else if (strcmp(cmd, "clone") == 0) {
        return cmd_clone(argc - 1, argv + 1);
    } else if (strcmp(cmd, "add") == 0) {
        return cmd_add(argc - 1, argv + 1);
    } else if (strcmp(cmd, "commit") == 0) {
        return cmd_commit(argc - 1, argv + 1);
    } else if (strcmp(cmd, "status") == 0) {
        return cmd_status(argc - 1, argv + 1);
    } else if (strcmp(cmd, "diff") == 0) {
        return cmd_diff(argc - 1, argv + 1);
    } else if (strcmp(cmd, "show") == 0) {
        return cmd_show(argc - 1, argv + 1);
    } else if (strcmp(cmd, "reset") == 0) {
        return cmd_reset(argc - 1, argv + 1);
    } else if (strcmp(cmd, "log") == 0) {
        return cmd_log(argc - 1, argv + 1);
    } else if (strcmp(cmd, "branch") == 0) {
        return cmd_branch(argc - 1, argv + 1);
    } else if (strcmp(cmd, "config") == 0) {
        return cmd_config(argc - 1, argv + 1);
    } else if (strcmp(cmd, "checkout") == 0) {
        return cmd_checkout(argc - 1, argv + 1);
    } else {
        fprintf(stderr, "cit: '%s' is not a cit command.\n", cmd);
        return 1;
    }

    return 0;
}
