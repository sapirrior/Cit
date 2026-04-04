#include "commands.h"
#include "utils.h"
#include "sha256.h"
#include "object.h"
#include "index.h"
#include "portability.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmd_init(int argc, char *argv[]) {
    char *cwd = getcwd(NULL, 0);
    if (dir_exists(".cit")) {
        printf("Reinitialized existing Cit repository in %s/.cit/\n", cwd);
        if (cwd) free(cwd);
        return 0;
    }

    if (mkdir_p(".cit/objects") != 0 ||
        mkdir_p(".cit/refs/heads") != 0) {
        fprintf(stderr, "Error: Could not create Cit repository structure.\n");
        if (cwd) free(cwd);
        return 1;
    }

    if (write_string_to_file(".cit/HEAD", "ref: refs/heads/main\n") != 0) {
        fprintf(stderr, "Error: Could not initialize .cit/HEAD.\n");
        if (cwd) free(cwd);
        return 1;
    }

    printf("Initialized empty Cit repository in %s/.cit/\n", cwd);
    if (cwd) free(cwd);
    return 0;
}

static int add_file(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open file %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    void *buf = malloc(size);
    if (!buf) {
        fclose(f);
        return -1;
    }
    fread(buf, 1, size, f);
    fclose(f);

    char *sha256_hex = write_object(buf, size, OBJ_BLOB);
    if (!sha256_hex) {
        free(buf);
        return -1;
    }

    uint8_t sha256_bytes[32];
    for (int i = 0; i < 32; i++) {
        sscanf(sha256_hex + (i * 2), "%02hhx", &sha256_bytes[i]);
    }

    add_to_index(index, path, sha256_bytes, (uint32_t)size);
    
    free(buf);
    free(sha256_hex);
    return 0;
}

static int add_recursive(Index *index, const char *path) {
    // Basic ignore of .cit folder
    if (strstr(path, ".cit")) return 0;

    if (is_file(path)) {
        return add_file(index, path);
    } else if (dir_exists(path)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char subpath[1024];
            if (strcmp(path, ".") == 0) {
                snprintf(subpath, sizeof(subpath), "%s", entry->d_name);
            } else {
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            }
            add_recursive(index, subpath);
        }
        closedir(dir);
    }
    return 0;
}

int cmd_add(int argc, char *argv[]) {
    if (argc < 1) {
        printf("Usage: cit add <path>\n");
        return 1;
    }

    Index *index = read_index();
    for (int i = 0; i < argc; i++) {
        add_recursive(index, argv[i]);
    }
    write_index(index);
    free_index(index);
    return 0;
}

int cmd_commit(int argc, char *argv[]) {
    if (!check_config()) {
        fprintf(stderr, "Error: User configuration (username and email) not found.\n");
        fprintf(stderr, "Use 'cit config -u <name>' and 'cit config -e <email>' to set them.\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: cit commit <message>\n");
        return 1;
    }

    char *message = argv[1];
    Index *index = read_index();
    if (index->count == 0) {
        printf("Nothing to commit (create/copy files and use \"cit add\")\n");
        free_index(index);
        return 0;
    }

    // 1. Create Tree object from Index
    size_t tree_size = 0;
    for (uint32_t i = 0; i < index->count; i++) {
        tree_size += 64 + 1 + strlen(index->entries[i].path) + 1;
    }

    char *tree_buf = malloc(tree_size + 1);
    if (!tree_buf) {
        free_index(index);
        return 1;
    }
    char *p = tree_buf;
    for (uint32_t i = 0; i < index->count; i++) {
        for (int j = 0; j < 32; j++) {
            p += sprintf(p, "%02x", index->entries[i].sha256[j]);
        }
        p += sprintf(p, " %s\n", index->entries[i].path);
    }
    
    char *tree_sha256 = write_object(tree_buf, tree_size, OBJ_TREE);
    free(tree_buf);
    if (!tree_sha256) {
        free_index(index);
        return 1;
    }

    // 2. Get parent commit SHA from HEAD
    char parent_sha256[65] = "";
    FILE *fhead = fopen(".cit/HEAD", "r");
    if (fhead) {
        char head_content[256];
        if (fgets(head_content, sizeof(head_content), fhead)) {
            if (strncmp(head_content, "ref: ", 5) == 0) {
                char ref_path[256];
                if (sscanf(head_content + 5, "%s", ref_path) == 1) {
                    char full_ref_path[512];
                    snprintf(full_ref_path, sizeof(full_ref_path), ".cit/%s", ref_path);
                    FILE *fref = fopen(full_ref_path, "r");
                    if (fref) {
                        if (!fgets(parent_sha256, 65, fref)) parent_sha256[0] = 0;
                        fclose(fref);
                    }
                }
            }
        }
        fclose(fhead);
    }

    // 3. Create Commit object
    size_t commit_buf_size = 128 + 64 + 64 + strlen(message);
    char *commit_buf = malloc(commit_buf_size);
    if (!commit_buf) {
        free(tree_sha256);
        free_index(index);
        return 1;
    }

    int commit_len;
    if (strlen(parent_sha256) > 0) {
        commit_len = sprintf(commit_buf, "tree %s\nparent %s\n\n%s\n", tree_sha256, parent_sha256, message);
    } else {
        commit_len = sprintf(commit_buf, "tree %s\n\n%s\n", tree_sha256, message);
    }

    char *commit_sha256 = write_object(commit_buf, commit_len, OBJ_COMMIT);
    free(commit_buf);

    if (!commit_sha256) {
        free(tree_sha256);
        free_index(index);
        return 1;
    }

    // 4. Update ref (branch)
    fhead = fopen(".cit/HEAD", "r");
    if (fhead) {
        char head_content[256];
        if (fgets(head_content, sizeof(head_content), fhead)) {
            if (strncmp(head_content, "ref: ", 5) == 0) {
                char ref_path[256];
                if (sscanf(head_content + 5, "%s", ref_path) == 1) {
                    char full_ref_path[512];
                    snprintf(full_ref_path, sizeof(full_ref_path), ".cit/%s", ref_path);
                    write_string_to_file(full_ref_path, commit_sha256);
                }
            }
        }
        fclose(fhead);
    }

    printf("[main %s] %s\n", commit_sha256, message);

    free(tree_sha256);
    free(commit_sha256);
    free_index(index);
    return 0;
}

static char *get_current_branch() {
    FILE *fhead = fopen(".cit/HEAD", "r");
    if (!fhead) return NULL;
    static char head_content[256];
    if (fgets(head_content, sizeof(head_content), fhead)) {
        if (strncmp(head_content, "ref: refs/heads/", 16) == 0) {
            char *branch = head_content + 16;
            // Trim whitespace
            char *p = branch;
            while (*p && *p != ' ' && *p != '\n' && *p != '\r') p++;
            *p = 0;
            fclose(fhead);
            return branch;
        }
    }
    fclose(fhead);
    return NULL;
}

static void scan_working_dir(const char *path, Index *index, char ***untracked, int *untracked_count, char ***modified, int *modified_count) {
    if (strstr(path, ".cit")) return;

    if (is_file(path)) {
        // Normalize path for comparison with index (strip leading "./")
        const char *norm_path = path;
        if (strncmp(path, "./", 2) == 0) norm_path = path + 2;

        int found_in_index = 0;
        for (uint32_t i = 0; i < index->count; i++) {
            if (strcmp(index->entries[i].path, norm_path) == 0) {
                found_in_index = 1;
                struct stat st;
                if (stat(path, &st) == 0) {
                    if ((uint32_t)st.st_mtime != index->entries[i].mtime || (uint32_t)st.st_size != index->entries[i].size) {
                        char **new_modified = realloc(*modified, sizeof(char *) * (*modified_count + 1));
                        if (new_modified) {
                            *modified = new_modified;
                            (*modified)[*modified_count] = strdup(norm_path);
                            (*modified_count)++;
                        }
                    }
                }
                break;
            }
        }
        if (!found_in_index) {
            char **new_untracked = realloc(*untracked, sizeof(char *) * (*untracked_count + 1));
            if (!new_untracked) return;
            *untracked = new_untracked;
            (*untracked)[*untracked_count] = strdup(norm_path);
            (*untracked_count)++;
        }
    } else if (dir_exists(path)) {
        DIR *dir = opendir(path);
        if (!dir) return;
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            char subpath[1024];
            if (strcmp(path, ".") == 0) {
                snprintf(subpath, sizeof(subpath), "%s", entry->d_name);
            } else {
                snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);
            }
            scan_working_dir(subpath, index, untracked, untracked_count, modified, modified_count);
        }
        closedir(dir);
    }
}

int cmd_status(int argc, char *argv[]) {
    Index *index = read_index();
    char *branch = get_current_branch();
    printf("On branch %s\n\n", branch ? branch : "main");

    char **untracked = NULL;
    int untracked_count = 0;
    char **modified = NULL;
    int modified_count = 0;
    char **deleted = NULL;
    int deleted_count = 0;

    // 1. Scan working dir for untracked and modified
    scan_working_dir(".", index, &untracked, &untracked_count, &modified, &modified_count);

    // 2. Check index for deleted files
    for (uint32_t i = 0; i < index->count; i++) {
        if (!is_file(index->entries[i].path)) {
            char **new_deleted = realloc(deleted, sizeof(char *) * (deleted_count + 1));
            if (new_deleted) {
                deleted = new_deleted;
                deleted[deleted_count] = strdup(index->entries[i].path);
                deleted_count++;
            }
        }
    }

    // 3. Output
    int has_changes = 0;

    if (index->count > 0) {
        int staged_visible = 0;
        for (uint32_t i = 0; i < index->count; i++) {
            // Only show as staged if it's not marked as deleted
            int is_deleted = 0;
            for (int j = 0; j < deleted_count; j++) {
                if (strcmp(index->entries[i].path, deleted[j]) == 0) {
                    is_deleted = 1;
                    break;
                }
            }
            if (!is_deleted) {
                if (!staged_visible) {
                    printf("Changes to be committed:\n");
                    printf("  (use \"cit commit\" to record these changes)\n");
                    staged_visible = 1;
                }
                printf("  \033[32mnew file: %s\033[0m\n", index->entries[i].path);
                has_changes = 1;
            }
        }
        if (staged_visible) printf("\n");
    }

    if (modified_count > 0 || deleted_count > 0) {
        printf("Changes not staged for commit:\n");
        printf("  (use \"cit add <file>...\" to update what will be committed)\n");
        for (int i = 0; i < modified_count; i++) {
            printf("  \033[31mmodified: %s\033[0m\n", modified[i]);
            free(modified[i]);
        }
        for (int i = 0; i < deleted_count; i++) {
            printf("  \033[31mdeleted:  %s\033[0m\n", deleted[i]);
            free(deleted[i]);
        }
        printf("\n");
        free(modified);
        free(deleted);
        has_changes = 1;
    }

    if (untracked_count > 0) {
        printf("Untracked files:\n");
        printf("  (use \"cit add <file>...\" to include in what will be committed)\n");
        for (int i = 0; i < untracked_count; i++) {
            printf("  \033[31m%s\033[0m\n", untracked[i]);
            free(untracked[i]);
        }
        printf("\n");
        free(untracked);
        has_changes = 1;
    }

    if (!has_changes) {
        printf("nothing to commit, working tree clean\n");
    }

    free_index(index);
    return 0;
}

int cmd_log(int argc, char *argv[]) {
    char current_sha256[65] = "";
    
    // 1. Get current commit from HEAD
    FILE *fhead = fopen(".cit/HEAD", "r");
    if (!fhead) {
        fprintf(stderr, "Error: Not a cit repository (or .cit/HEAD missing)\n");
        return 1;
    }
    
    char head_content[256];
    if (fgets(head_content, sizeof(head_content), fhead)) {
        if (strncmp(head_content, "ref: ", 5) == 0) {
            char ref_path[256];
            sscanf(head_content + 5, "%s", ref_path);
            char full_ref_path[512];
            snprintf(full_ref_path, sizeof(full_ref_path), ".cit/%s", ref_path);
            FILE *fref = fopen(full_ref_path, "r");
            if (fref) {
                fgets(current_sha256, 65, fref);
                fclose(fref);
            }
        }
    }
    fclose(fhead);

    if (strlen(current_sha256) == 0) {
        printf("No commits yet.\n");
        return 0;
    }

    // 2. Traverse parent commits
    while (strlen(current_sha256) > 0) {
        size_t obj_len;
        obj_type type;
        char *content = read_object(current_sha256, &obj_len, &type);
        if (!content || type != OBJ_COMMIT) {
            if (content) free(content);
            break;
        }

        printf("\033[33mcommit %s\033[0m\n", current_sha256);
        
        // Simple parsing of commit content
        char *message = strstr(content, "\n\n");
        if (message) {
            message += 2;
            printf("    %s", message);
        }
        printf("\n");

        // Find parent
        char *parent_ptr = strstr(content, "parent ");
        if (parent_ptr) {
            strncpy(current_sha256, parent_ptr + 7, 64);
            current_sha256[64] = 0;
        } else {
            current_sha256[0] = 0;
        }
        
        free(content);
    }

    return 0;
}

int cmd_branch(int argc, char *argv[]) {
    if (argc > 1 && !check_config()) {
        fprintf(stderr, "Error: User configuration (username and email) not found.\n");
        fprintf(stderr, "Use 'cit config -u <name>' and 'cit config -e <email>' to set them.\n");
        return 1;
    }

    if (argc == 1) {
        // List branches
        DIR *dir = opendir(".cit/refs/heads");
        if (!dir) return 1;
        char *current = get_current_branch();
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;
            if (current && strcmp(entry->d_name, current) == 0) {
                printf("* \033[32m%s\033[0m\n", entry->d_name);
            } else {
                printf("  %s\n", entry->d_name);
            }
        }
        closedir(dir);
        return 0;
    }

    if (argc == 2 && strcmp(argv[1], "-d") != 0 && strcmp(argv[1], "-m") != 0) {
        // Create branch
        char *new_branch = argv[1];
        char current_sha[65] = "";
        char *current = get_current_branch();
        if (current) {
            char full_ref_path[512];
            snprintf(full_ref_path, sizeof(full_ref_path), ".cit/refs/heads/%s", current);
            FILE *fref = fopen(full_ref_path, "r");
            if (fref) {
                if (!fgets(current_sha, 65, fref)) {
                    current_sha[0] = 0;
                }
                fclose(fref);
            }
        }

        if (strlen(current_sha) == 0) {
            fprintf(stderr, "Error: No commits yet. Cannot create branch.\n");
            return 1;
        }

        char new_ref_path[512];
        snprintf(new_ref_path, sizeof(new_ref_path), ".cit/refs/heads/%s", new_branch);
        if (is_file(new_ref_path)) {
            fprintf(stderr, "Error: Branch %s already exists.\n", new_branch);
            return 1;
        }

        if (write_string_to_file(new_ref_path, current_sha) != 0) {
            fprintf(stderr, "Error: Could not create branch %s.\n", new_branch);
            return 1;
        }
        return 0;
    }

    if (argc == 3 && strcmp(argv[1], "-d") == 0) {
        // Delete branch
        char *target = argv[2];
        char *current = get_current_branch();
        if (current && strcmp(target, current) == 0) {
            fprintf(stderr, "Error: Cannot delete the branch you are currently on.\n");
            return 1;
        }
        char path[512];
        snprintf(path, sizeof(path), ".cit/refs/heads/%s", target);
        if (remove(path) != 0) {
            fprintf(stderr, "Error: Could not delete branch %s.\n", target);
            return 1;
        }
        printf("Deleted branch %s.\n", target);
        return 0;
    }

    if (argc == 4 && strcmp(argv[1], "-m") == 0) {
        // Rename branch
        char *old_name = argv[2];
        char *new_name = argv[3];
        char old_path[512], new_path[512];
        snprintf(old_path, sizeof(old_path), ".cit/refs/heads/%s", old_name);
        snprintf(new_path, sizeof(new_path), ".cit/refs/heads/%s", new_name);
        
        if (rename(old_path, new_path) != 0) {
            fprintf(stderr, "Error: Could not rename branch %s to %s.\n", old_name, new_name);
            return 1;
        }
        
        // If we renamed the current branch, update HEAD
        char *current = get_current_branch();
        if (current && strcmp(old_name, current) == 0) {
            char head_content[256];
            snprintf(head_content, sizeof(head_content), "ref: refs/heads/%s\n", new_name);
            write_string_to_file(".cit/HEAD", head_content);
        }
        return 0;
    }

    printf("Usage: cit branch [<name> | -d <name> | -m <old> <new>]\n");
    return 1;
}

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
