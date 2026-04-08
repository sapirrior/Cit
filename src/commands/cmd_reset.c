#include "commands.h"
#include "index.h"
#include "object.h"
#include "ui.h"
#include "refs.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_reset(int argc, char *argv[]) {
    if (argc < 2) {
        ui_error("Usage: cit reset [--hard|--soft] <commit/file>");
        return 1;
    }

    Index *index = read_index();
    if (!index) return 1;

    // Simple implementation of unstaging a file
    char *target = argv[1];
    int found = 0;
    for (uint32_t i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, target) == 0) {
            // Remove entry from index
            for (uint32_t j = i; j < index->count - 1; j++) {
                index->entries[j] = index->entries[j + 1];
            }
            index->count--;
            found = 1;
            ui_info("Unstaged %s", target);
            break;
        }
    }

    if (found) {
        write_index(index);
    } else {
        // TODO: Implement reset to commit
        ui_warn("Commit reset not fully implemented. Currently only unstaging files works.");
    }

    free_index(index);
    return 0;
}
