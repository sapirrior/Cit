#include "commands.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_config(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: cit config -u/-username <name> OR cit config -e/-email <email>\n");
        return 1;
    }

    char *path = get_config_path();
    if (!path) return 1;

    // Create .citconfig directory if it doesn't exist
    char dir[512];
    strcpy(dir, path);
    char *last_slash = strrchr(dir, '/');
    if (last_slash) {
        *last_slash = 0;
        mkdir_p(dir);
    }

    char username[256] = "";
    char email[256] = "";

    // Read existing config
    FILE *f = fopen(path, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "username=", 9) == 0) {
                sscanf(line + 9, "%s", username);
            } else if (strncmp(line, "email=", 6) == 0) {
                sscanf(line + 6, "%s", email);
            }
        }
        fclose(f);
    }

    if (strcmp(argv[1], "-u") == 0 || strcmp(argv[1], "-username") == 0) {
        strncpy(username, argv[2], sizeof(username) - 1);
    } else if (strcmp(argv[1], "-e") == 0 || strcmp(argv[1], "-email") == 0) {
        if (!is_valid_email(argv[2])) {
            fprintf(stderr, "Error: Invalid email format.\n");
            return 1;
        }
        strncpy(email, argv[2], sizeof(email) - 1);
    } else {
        printf("Usage: cit config -u/-username <name> OR cit config -e/-email <email>\n");
        return 1;
    }

    f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "Error: Could not write to config file.\n");
        return 1;
    }
    fprintf(f, "[Cit Configuration]\n");
    fprintf(f, "username=%s\n", username);
    fprintf(f, "email=%s\n", email);
    fclose(f);

    printf("Configuration updated.\n");
    return 0;
}
