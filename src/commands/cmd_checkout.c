#include "commands.h"
#include "object.h"
#include "index.h"
#include "utils.h"
#include "portability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_checkout(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: cit checkout <commit-sha> [<path>]\n");
        return 1;
    }

    char *target_sha = argv[1];
    char *target_path = (argc > 2) ? argv[2] : NULL;

    // 1. Get Tree SHA from Commit
    size_t obj_len;
    obj_type type;
    char *commit_content = read_object(target_sha, &obj_len, &type);
    if (!commit_content) {
        fprintf(stderr, "Error: Could not read commit %s. Check if it exists.\n", target_sha);
        return 1;
    }
    if (type != OBJ_COMMIT) {
        fprintf(stderr, "Error: Object %s is not a commit.\n", target_sha);
        free(commit_content);
        return 1;
    }

    char tree_sha[65] = "";
    char *tree_line = strstr(commit_content, "tree ");
    if (tree_line) {
        strncpy(tree_sha, tree_line + 5, 64);
        tree_sha[64] = 0;
    }
    free(commit_content);

    if (strlen(tree_sha) == 0) {
        fprintf(stderr, "Error: Could not find tree in commit %s\n", target_sha);
        return 1;
    }

    // 2. Read Tree object
    char *tree_content = read_object(tree_sha, &obj_len, &type);
    if (!tree_content) {
        fprintf(stderr, "Error: Could not read tree %s\n", tree_sha);
        return 1;
    }
    if (type != OBJ_TREE) {
        fprintf(stderr, "Error: Object %s is not a tree.\n", tree_sha);
        free(tree_content);
        return 1;
    }

    // 3. Parse tree and identify files to restore
    typedef struct {
        char sha[65];
        char path[1024];
    } TreeEntry;
    
    uint32_t entry_cap = 128;
    uint32_t entry_count = 0;
    TreeEntry *entries = malloc(sizeof(TreeEntry) * entry_cap);
    if (!entries) {
        free(tree_content);
        return 1;
    }

    char *saveptr;
    char *line = strtok_r(tree_content, "\n", &saveptr);
    while (line) {
        if (entry_count >= entry_cap) {
            entry_cap *= 2;
            TreeEntry *new_entries = realloc(entries, sizeof(TreeEntry) * entry_cap);
            if (!new_entries) {
                free(tree_content);
                free(entries);
                return 1;
            }
            entries = new_entries;
        }
        
        if (sscanf(line, "%64s %1023s", entries[entry_count].sha, entries[entry_count].path) == 2) {
            // Filter if target_path is specified
            if (!target_path || strcmp(entries[entry_count].path, target_path) == 0) {
                entry_count++;
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    if (entry_count == 0) {
        if (target_path) printf("Path %s not found in commit %s\n", target_path, target_sha);
        else printf("Commit tree is empty.\n");
        free(tree_content);
        free(entries);
        return 0;
    }

    // 4. Confirmation
    printf("Following files will be restored/overwritten:\n");
    for (uint32_t i = 0; i < entry_count; i++) {
        printf("  %s\n", entries[i].path);
    }
    printf("\nAre you sure you want to proceed? (y/n): ");
    char choice[10];
    if (!fgets(choice, sizeof(choice), stdin) || (choice[0] != 'y' && choice[0] != 'Y')) {
        printf("Checkout cancelled.\n");
        free(tree_content);
        free(entries);
        return 0;
    }

    // 5. Restore files
    Index *index = read_index();
    for (uint32_t i = 0; i < entry_count; i++) {
        size_t blob_len;
        obj_type btype;
        void *blob_data = read_object(entries[i].sha, &blob_len, &btype);
        if (blob_data && btype == OBJ_BLOB) {
            // Ensure parent directory exists
            char *last_slash = strrchr(entries[i].path, '/');
            if (last_slash) {
                char dpath[1024];
                strncpy(dpath, entries[i].path, last_slash - entries[i].path);
                dpath[last_slash - entries[i].path] = 0;
                mkdir_p(dpath);
            }

            FILE *fw = fopen(entries[i].path, "wb");
            if (fw) {
                if (fwrite(blob_data, 1, blob_len, fw) != blob_len) {
                    fprintf(stderr, "Warning: Could not write all data to %s\n", entries[i].path);
                }
                fclose(fw);
                printf("Restored %s\n", entries[i].path);
                
                // Update index to match checkout
                uint8_t sha_bytes[32];
                for (int j = 0; j < 32; j++) {
                    sscanf(entries[i].sha + (j * 2), "%02hhx", &sha_bytes[j]);
                }
                add_to_index(index, entries[i].path, sha_bytes, (uint32_t)blob_len);
            } else {
                fprintf(stderr, "Error: Could not open %s for writing.\n", entries[i].path);
            }
            free(blob_data);
        } else {
            fprintf(stderr, "Error: Could not read blob %s for %s\n", entries[i].sha, entries[i].path);
            if (blob_data) free(blob_data);
        }
    }
    write_index(index);
    free_index(index);

    free(tree_content);
    free(entries);
    printf("Checkout completed.\n");
    return 0;
}
