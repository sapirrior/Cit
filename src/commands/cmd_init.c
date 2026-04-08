#include "commands.h"
#include "utils.h"
#include "portability.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>

int cmd_init(int argc, char *argv[]) {
    char *cwd = getcwd(NULL, 0);
    if (dir_exists(".cit")) {
        ui_success("Reinitialized existing Cit repository in %s/.cit/", cwd);
        if (cwd) free(cwd);
        return 0;
    }

    if (mkdir_p(".cit/objects") != 0 ||
        mkdir_p(".cit/refs/heads") != 0) {
        ui_error("Could not create Cit repository structure.");
        if (cwd) free(cwd);
        return 1;
    }

    if (write_string_to_file(".cit/HEAD", "ref: refs/heads/main\n") != 0) {
        ui_error("Could not initialize .cit/HEAD.");
        if (cwd) free(cwd);
        return 1;
    }

    ui_success("Initialized empty Cit repository in %s/.cit/", cwd);
    if (cwd) free(cwd);
    return 0;
}
