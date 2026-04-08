#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int check_config() {
    char *path = get_config_path();
    if (!path || !is_file(path)) return 0;

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
    int has_user = 0, has_email = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "username=", 9) == 0) has_user = 1;
        if (strncmp(line, "email=", 6) == 0) has_email = 1;
    }
    fclose(f);
    return has_user && has_email;
}
