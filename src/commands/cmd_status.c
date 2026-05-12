#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include "portability.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    char **staged_new;
    int staged_new_count;
    char **staged_mod;
    int staged_mod_count;
    char **staged_del;
    int staged_del_count;
    char **unstaged_mod;
    int unstaged_mod_count;
    char **unstaged_del;
    int unstaged_del_count;
    char **untracked;
    int untracked_count;
} StatusResult;

static void add_to_list(char ***list, int *count, const char *path) {
    char **new_list = realloc(*list, sizeof(char *) * (*count + 1));
    if (new_list) {
        *list = new_list;
        (*list)[*count] = strdup(path);
        (*count)++;
    }
}

static void free_status_result(StatusResult *res) {
    for (int i = 0; i < res->staged_new_count; i++) free(res->staged_new[i]);
    free(res->staged_new);
    for (int i = 0; i < res->staged_mod_count; i++) free(res->staged_mod[i]);
    free(res->staged_mod);
    for (int i = 0; i < res->staged_del_count; i++) free(res->staged_del[i]);
    free(res->staged_del);
    for (int i = 0; i < res->unstaged_mod_count; i++) free(res->unstaged_mod[i]);
    free(res->unstaged_mod);
    for (int i = 0; i < res->unstaged_del_count; i++) free(res->unstaged_del[i]);
    free(res->unstaged_del);
    for (int i = 0; i < res->untracked_count; i++) free(res->untracked[i]);
    free(res->untracked);
}

// Compare HEAD tree to index (Staged Changes)
static void compare_tree_to_index(const char *tree_sha, const char *prefix, Index *index, uint32_t *index_idx, StatusResult *res) {
    size_t obj_len;
    obj_type type;
    char *tree_content = read_object(tree_sha, &obj_len, &type);
    if (!tree_content || type != OBJ_TREE) {
        free(tree_content);
        return;
    }

    char *saveptr;
    char *line = strtok_r(tree_content, "\n", &saveptr);
    while (line) {
        char type_str[10], sha[65], name[256];
        if (sscanf(line, "%9s %64s %255s", type_str, sha, name) == 3) {
            char full_path[1024];
            if (strlen(prefix) == 0) snprintf(full_path, sizeof(full_path), "%s", name);
            else snprintf(full_path, sizeof(full_path), "%s/%s", prefix, name);

            if (strcmp(type_str, "blob") == 0) {
                // Find in index
                while (*index_idx < index->count && strcmp(index->entries[*index_idx].path, full_path) < 0) {
                    // Path in index but not in tree (staged new)
                    add_to_list(&res->staged_new, &res->staged_new_count, index->entries[*index_idx].path);
                    (*index_idx)++;
                }

                if (*index_idx < index->count && strcmp(index->entries[*index_idx].path, full_path) == 0) {
                    // Compare SHA
                    char index_sha[65];
                    for (int j = 0; j < 32; j++) snprintf(index_sha + (j * 2), 3, "%02x", index->entries[*index_idx].sha256[j]);
                    if (strcmp(sha, index_sha) != 0) {
                        add_to_list(&res->staged_mod, &res->staged_mod_count, full_path);
                    }
                    (*index_idx)++;
                } else {
                    // Path in tree but not in index (staged deleted)
                    add_to_list(&res->staged_del, &res->staged_del_count, full_path);
                }
            } else if (strcmp(type_str, "tree") == 0) {
                compare_tree_to_index(sha, full_path, index, index_idx, res);
            }
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }
    free(tree_content);
}

// Compare Working Directory to Index (Unstaged Changes)
static void compare_dir_to_index(const char *path, Index *index, uint32_t *index_idx, StatusResult *res) {
    if (strstr(path, ".cit")) return;

    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char **entries = NULL;
    int entry_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        entries = realloc(entries, sizeof(char *) * (entry_count + 1));
        entries[entry_count++] = strdup(entry->d_name);
    }
    closedir(dir);

    // Sort directory entries to match index order
    if (entry_count > 1) {
        for (int i = 0; i < entry_count - 1; i++) {
            for (int j = i + 1; j < entry_count; j++) {
                if (strcmp(entries[i], entries[j]) > 0) {
                    char *tmp = entries[i];
                    entries[i] = entries[j];
                    entries[j] = tmp;
                }
            }
        }
    }

    for (int i = 0; i < entry_count; i++) {
        char full_path[1024];
        if (strcmp(path, ".") == 0) snprintf(full_path, sizeof(full_path), "%s", entries[i]);
        else snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]);

        if (is_file(full_path)) {
            while (*index_idx < index->count && strcmp(index->entries[*index_idx].path, full_path) < 0) {
                // Index has it, but it's not in this directory scan (could be in another dir or deleted)
                // We handle deletions separately to avoid confusion during recursion
                (*index_idx)++;
            }

            if (*index_idx < index->count && strcmp(index->entries[*index_idx].path, full_path) == 0) {
                // Compare stat
                struct stat st;
                if (stat(full_path, &st) == 0) {
                    if ((uint32_t)st.st_mtime != index->entries[*index_idx].mtime || (uint32_t)st.st_size != index->entries[*index_idx].size) {
                        add_to_list(&res->unstaged_mod, &res->unstaged_mod_count, full_path);
                    }
                }
                (*index_idx)++;
            } else {
                // Not in index (untracked)
                add_to_list(&res->untracked, &res->untracked_count, full_path);
            }
        } else if (dir_exists(full_path)) {
            compare_dir_to_index(full_path, index, index_idx, res);
        }
        free(entries[i]);
    }
    free(entries);
}

int cmd_status(int argc, char *argv[]) {
    Index *index = read_index();
    char *branch = get_current_branch();
    
    printf(COLOR_BOLD "On branch " COLOR_FG_ACCENT "%s" COLOR_RESET "\n", branch ? branch : "main");

    StatusResult res = {0};
    char *head_sha = get_ref_sha(branch ? branch : "main");
    
    // 1. Staged Changes (Tree vs Index)
    if (head_sha) {
        size_t obj_len;
        obj_type type;
        char *commit_content = read_object(head_sha, &obj_len, &type);
        if (commit_content && type == OBJ_COMMIT) {
            char tree_sha[65] = "";
            char *tree_line = strstr(commit_content, "tree ");
            if (tree_line) {
                strncpy(tree_sha, tree_line + 5, 64);
                tree_sha[64] = 0;
                uint32_t idx = 0;
                compare_tree_to_index(tree_sha, "", index, &idx, &res);
                // Any remaining in index are staged new
                while (idx < index->count) {
                    add_to_list(&res.staged_new, &res.staged_new_count, index->entries[idx].path);
                    idx++;
                }
            }
        }
        free(commit_content);
    } else {
        // No HEAD, all index entries are staged new
        for (uint32_t i = 0; i < index->count; i++) {
            add_to_list(&res.staged_new, &res.staged_new_count, index->entries[i].path);
        }
    }

    // 2. Unstaged/Untracked Changes (Dir vs Index)
    uint32_t idx = 0;
    compare_dir_to_index(".", index, &idx, &res);
    
    // 3. Unstaged Deletions (Index paths that don't exist)
    for (uint32_t i = 0; i < index->count; i++) {
        if (!is_file(index->entries[i].path)) {
            add_to_list(&res.unstaged_del, &res.unstaged_del_count, index->entries[i].path);
        }
    }

    int has_changes = 0;

    // Output Staged
    if (res.staged_new_count > 0 || res.staged_mod_count > 0 || res.staged_del_count > 0) {
        ui_header("Changes to be committed:");
        ui_info("  (use \"cit commit\" to record these changes)");
        for (int i = 0; i < res.staged_new_count; i++) printf("  " COLOR_FG_SUCCESS "new file:   %s" COLOR_RESET "\n", res.staged_new[i]);
        for (int i = 0; i < res.staged_mod_count; i++) printf("  " COLOR_FG_SUCCESS "modified:   %s" COLOR_RESET "\n", res.staged_mod[i]);
        for (int i = 0; i < res.staged_del_count; i++) printf("  " COLOR_FG_SUCCESS "deleted:    %s" COLOR_RESET "\n", res.staged_del[i]);
        printf("\n");
        has_changes = 1;
    }

    // Output Unstaged
    if (res.unstaged_mod_count > 0 || res.unstaged_del_count > 0) {
        ui_header("Changes not staged for commit:");
        ui_info("  (use \"cit add <file>...\" to update what will be committed)");
        for (int i = 0; i < res.unstaged_mod_count; i++) printf("  " COLOR_FG_DANGER "modified:   %s" COLOR_RESET "\n", res.unstaged_mod[i]);
        for (int i = 0; i < res.unstaged_del_count; i++) printf("  " COLOR_FG_DANGER "deleted:    %s" COLOR_RESET "\n", res.unstaged_del[i]);
        printf("\n");
        has_changes = 1;
    }

    // Output Untracked
    if (res.untracked_count > 0) {
        ui_header("Untracked files:");
        ui_info("  (use \"cit add <file>...\" to include in what will be committed)");
        for (int i = 0; i < res.untracked_count; i++) printf("  " COLOR_FG_DANGER "%s" COLOR_RESET "\n", res.untracked[i]);
        printf("\n");
        has_changes = 1;
    }

    if (!has_changes) {
        ui_info("nothing to commit, working tree clean");
    }

    free_status_result(&res);
    free_index(index);
    return 0;
}
