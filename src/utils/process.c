#include "utils.h"
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

int safe_execute(const char *command, char *const argv[]) {
#ifdef _WIN32
    // On Windows, fallback to system() for now or implement CreateProcess
    // Note: This is less secure but keeps basic portability for this phase.
    char cmd_line[1024] = {0};
    for (int i = 0; argv[i] != NULL; i++) {
        strncat(cmd_line, argv[i], sizeof(cmd_line) - strlen(cmd_line) - 1);
        strncat(cmd_line, " ", sizeof(cmd_line) - strlen(cmd_line) - 1);
    }
    return system(cmd_line);
#else
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) {
        execvp(command, argv);
        exit(127); // exec failed
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return -1;
    }
#endif
}
