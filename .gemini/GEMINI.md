# Cit: Project Technical Specifications & System Mandates

## Core Philosophy
Cit is a high-performance, local-first version control system written in C, designed for speed, security, and portability. It serves as a modern alternative to Git, replacing SHA-1 with SHA-256 and optimizing for local filesystem operations.

## Architecture & Modular Structure
The codebase follows a strict modular design:
- **`src/`**: Implementation files (`.c`).
- **`src/include/`**: Header files (`.h`).
- **`objects/`**: Compiled object files (`.o`), kept separate from source to maintain a clean tree.
- **`Makefile`**: Standard build system linking against `zlib` (`-lz`).

### Core Modules
- **`main.c`**: CLI entry point, command dispatcher, and robust help system (`-h`, `--help`).
- **`commands.c`**: Core logic for all CLI commands, including a sophisticated status engine.
- **`object.c`**: Content-addressable storage. Uses `zlib` for compression and a dynamic `inflate` buffer for reading objects of any size.
- **`index.c`**: Binary staging area management. Tracks file paths, SHA-256 hashes, sizes, and `mtime`.
- **`sha256.c`**: Independent, dependency-free SHA-256 implementation.
- **`utils.c`**: System-level utilities, robust email validation, and path normalization logic.
- **`portability.h`**: POSIX-compliant abstraction layer for cross-platform support (Linux, macOS, Android, Windows).

## Technical Implementation Details

### Data Storage & Integrity
- **Objects**: Compressed blobs stored at `.cit/objects/[first-2-chars-sha]/[rest-of-sha]`.
- **Refs**: Branch pointers at `.cit/refs/heads/[branch-name]`.
- **HEAD**: Pointer to current branch at `.cit/HEAD` (Format: `ref: refs/heads/[branch]`).
- **Index**: Binary file at `.cit/index` containing `IndexEntry` structs.
- **SHA-256**: All content is identified by its SHA-256 hash for absolute data integrity.

### Hardened Logic & Security
- **Memory Safety**: Mandatory `NULL` pointer checks on all `malloc` and `realloc` operations across the entire codebase.
- **Dynamic Commit Buffers**: `cmd_commit` utilizes dynamic memory for commit messages, removing fixed-length limitations and preventing buffer overflows.
- **Robust Email Validation**: Strict rules (no leading digits, valid TLD length >= 2, single '@') enforced during configuration.
- **Safe Operations**: Destructive `checkout` operations require explicit `y/n` confirmation.
- **Path Normalization**: `status` and `add` operations strip leading `./` prefixes to ensure consistent path matching regardless of how files are targeted.

### Resource Management
- **Streaming Decompression**: `read_object` uses `zlib`'s `inflate` with an auto-doubling buffer (starting at 1MB) to handle large objects without memory exhaustion.
- **Unlimited File Support**: `cmd_status` and `cmd_checkout` use dynamic memory to track an unlimited number of files in a single repository.
- **Error Propagation**: All recursive operations properly propagate error codes and ensure memory cleanup in all exit paths.

## CLI Reference

### Repository & Config
- `cit init`: Initialize repository.
- `cit config -u <name>`: Set global username.
- `cit config -e <email>`: Set global email (validated).
- `cit -h` / `cit --help`: Show detailed help message.

### Staging & Status
- `cit add <path>`: Stage files/directories recursively.
- `cit status`: Comprehensive status report:
  - **Staged (Green)**: Changes ready for commit.
  - **Modified (Red)**: Changes in working directory (detected via `mtime`/`size` comparison).
  - **Untracked (Red)**: New files not yet added.
  - **Deleted (Red)**: Tracked files missing from disk.

### History & Branching
- `cit commit "<msg>"`: Create a new snapshot of the index.
- `cit log`: View scrollable commit history.
- `cit branch`: List all branches (current marked with `*`).
- `cit branch <name>`: Create a new branch.
- `cit branch -d <name>`: Delete a branch.
- `cit branch -m <old> <new>`: Rename a branch.

### State Restoration
- `cit checkout <sha> [<path>]`: Restore entire commit or a specific file. Requires confirmation.

## Portability Layer
- **Windows**: Uses `_mkdir`, `_getcwd`, and `USERPROFILE` for config.
- **Unix/POSIX**: Uses `mkdir`, `getcwd`, and `HOME`.
- **Path Separators**: Logic handles both `/` and `\` to ensure cross-OS compatibility.
