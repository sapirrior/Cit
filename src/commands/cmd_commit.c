#include "commands.h"
#include "index.h"
#include "object.h"
#include "utils.h"
#include "config.h"
#include "refs.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STACK 32

typedef struct {
    char name[256];
    char *buf;
    size_t size;
    size_t cap;
} TreeStack;

static void rollback_objects(char **shas, int count) {
    for (int i = 0; i < count; i++) {
        char path[512];
        snprintf(path, sizeof(path), ".cit/objects/%.2s/%s", shas[i], shas[i] + 2);
        remove(path);
        free(shas[i]);
    }
    free(shas);
}

static char *tracked_write_object(const void *buf, size_t len, obj_type type, char ***tracked_shas, int *tracked_count) {
    char *sha = write_object(buf, len, type);
    if (sha) {
        char **new_shas = realloc(*tracked_shas, sizeof(char *) * (*tracked_count + 1));
        if (!new_shas) {
            free(sha);
            return NULL;
        }
        *tracked_shas = new_shas;
        (*tracked_shas)[*tracked_count] = strdup(sha);
        (*tracked_count)++;
    }
    return sha;
}

static void tree_stack_push(TreeStack *stack, int *depth, const char *name) {
    (*depth)++;
    strncpy(stack[*depth].name, name, 255);
    stack[*depth].name[255] = '\0';
    stack[*depth].cap = 4096;
    stack[*depth].size = 0;
    stack[*depth].buf = malloc(stack[*depth].cap);
}

int cmd_commit(int argc, char *argv[]) {
    if (!check_config()) {
        fprintf(stderr, "Error: User configuration (username and email) not found.\n");
        return 1;
    }

    if (argc < 2) {
        printf("Usage: cit commit <message>\n");
        return 1;
    }

    char *message = argv[1];
    Index *index = read_index();
    if (index->count == 0) {
        printf("Nothing to commit\n");
        free_index(index);
        return 0;
    }

    char **created_shas = NULL;
    int created_count = 0;

    // 1. Build hierarchical trees
    TreeStack stack[MAX_STACK];
    int depth = -1;

    // Push root tree
    tree_stack_push(stack, &depth, "");

    for (uint32_t i = 0; i < index->count; i++) {
        char path[1024];
        strncpy(path, index->entries[i].path, 1023);
        path[1023] = '\0';

        char *parts[MAX_STACK];
        int part_count = 0;
        char *token = strtok(path, "/");
        while (token && part_count < MAX_STACK) {
            parts[part_count++] = token;
            token = strtok(NULL, "/");
        }

        // Pop stack until we find a common ancestor
        int common = 0;
        while (common < depth && common < part_count - 1 && strcmp(stack[common + 1].name, parts[common]) == 0) {
            common++;
        }

        while (depth > common) {
            char *sha = tracked_write_object(stack[depth].buf, stack[depth].size, OBJ_TREE, &created_shas, &created_count);
            if (!sha) {
                rollback_objects(created_shas, created_count);
                for (int j = 0; j <= depth; j++) free(stack[j].buf);
                free_index(index);
                return 1;
            }
            char entry[512];
            int entry_len = snprintf(entry, sizeof(entry), "tree %s %s\n", sha, stack[depth].name);
            free(sha);
            
            free(stack[depth].buf);
            depth--;
            
            if (stack[depth].size + entry_len >= stack[depth].cap) {
                size_t new_cap = stack[depth].cap * 2;
                char *new_buf = realloc(stack[depth].buf, new_cap);
                if (!new_buf) {
                    rollback_objects(created_shas, created_count);
                    for (int j = 0; j <= depth; j++) free(stack[j].buf);
                    free(sha);
                    free_index(index);
                    return 1;
                }
                stack[depth].buf = new_buf;
                stack[depth].cap = new_cap;
            }
            memcpy(stack[depth].buf + stack[depth].size, entry, entry_len);
            stack[depth].size += entry_len;
        }

        // Push new directories
        for (int j = depth; j < part_count - 1; j++) {
            tree_stack_push(stack, &depth, parts[j]);
        }

        // Add blob to current top tree
        char sha_hex[65];
        for (int j = 0; j < 32; j++) snprintf(sha_hex + (j * 2), 3, "%02x", index->entries[i].sha256[j]);
        
        char entry[512];
        int entry_len = snprintf(entry, sizeof(entry), "blob %s %s\n", sha_hex, parts[part_count - 1]);
        if (stack[depth].size + entry_len >= stack[depth].cap) {
            size_t new_cap = stack[depth].cap * 2;
            char *new_buf = realloc(stack[depth].buf, new_cap);
            if (!new_buf) {
                rollback_objects(created_shas, created_count);
                for (int j = 0; j <= depth; j++) free(stack[j].buf);
                free_index(index);
                return 1;
            }
            stack[depth].buf = new_buf;
            stack[depth].cap = new_cap;
        }
        memcpy(stack[depth].buf + stack[depth].size, entry, entry_len);
        stack[depth].size += entry_len;
    }

    // Finalize all remaining trees
    while (depth > 0) {
        char *sha = tracked_write_object(stack[depth].buf, stack[depth].size, OBJ_TREE, &created_shas, &created_count);
        if (!sha) {
            rollback_objects(created_shas, created_count);
            for (int j = 0; j <= depth; j++) free(stack[j].buf);
            free_index(index);
            return 1;
        }
        char entry[512];
        int entry_len = snprintf(entry, sizeof(entry), "tree %s %s\n", sha, stack[depth].name);
        free(sha);
        
        free(stack[depth].buf);
        depth--;
        
        if (stack[depth].size + entry_len >= stack[depth].cap) {
            size_t new_cap = stack[depth].cap * 2;
            char *new_buf = realloc(stack[depth].buf, new_cap);
            if (!new_buf) {
                rollback_objects(created_shas, created_count);
                for (int j = 0; j <= depth; j++) free(stack[j].buf);
                free_index(index);
                return 1;
            }
            stack[depth].buf = new_buf;
            stack[depth].cap = new_cap;
        }
        memcpy(stack[depth].buf + stack[depth].size, entry, entry_len);
        stack[depth].size += entry_len;
    }

    char *tree_sha = tracked_write_object(stack[0].buf, stack[0].size, OBJ_TREE, &created_shas, &created_count);
    free(stack[0].buf);
    if (!tree_sha) {
        rollback_objects(created_shas, created_count);
        free_index(index);
        return 1;
    }

    // 2. Get parent commit
    char parent_sha[65] = "";
    char *current_branch = get_current_branch();
    if (current_branch) {
        char *p_sha = get_ref_sha(current_branch);
        if (p_sha) strncpy(parent_sha, p_sha, 64);
    }

    // 3. Create Commit object
    size_t commit_buf_size = 256 + strlen(message);
    char *commit_buf = malloc(commit_buf_size);
    int commit_len;
    if (strlen(parent_sha) > 0) {
        commit_len = snprintf(commit_buf, commit_buf_size, "tree %s\nparent %s\n\n%s\n", tree_sha, parent_sha, message);
    } else {
        commit_len = snprintf(commit_buf, commit_buf_size, "tree %s\n\n%s\n", tree_sha, message);
    }

    char *commit_sha = tracked_write_object(commit_buf, commit_len, OBJ_COMMIT, &created_shas, &created_count);
    free(commit_buf);
    if (!commit_sha) {
        rollback_objects(created_shas, created_count);
        free(tree_sha);
        free_index(index);
        return 1;
    }

    // 4. Update ref
    if (current_branch) {
        char ref_path[512];
        snprintf(ref_path, sizeof(ref_path), "refs/heads/%s", current_branch);
        update_ref(ref_path, commit_sha);
    }

    printf("[" COLOR_FG_ACCENT "%s" COLOR_RESET " " COLOR_FG_ATTENTION "%.7s" COLOR_RESET "] %s\n", current_branch ? current_branch : "detached", commit_sha, message);

    // Clean up tracked SHAs (they are successfully committed)
    for (int i = 0; i < created_count; i++) free(created_shas[i]);
    free(created_shas);
    free(tree_sha);
    free(commit_sha);
    free_index(index);
    return 0;
}
