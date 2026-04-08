# Cit: Project Technical Specifications & System Mandates

## Core Philosophy
Cit is a high-performance, local-first version control system written in C, designed for speed, security, and portability. It serves as a modern alternative to Git, replacing SHA-1 with SHA-256 and optimizing for local filesystem operations.

## Architecture & Modular Structure
The codebase follows a strict modular design:
- **`src/`**: Implementation files (`.c`).
- **`src/core/`**: Core logic (Object store, Index, Hashing, Config, Refs).
- **`src/commands/`**: CLI command implementations.
- **`src/ui/`**: User interface and themed output.
- **`src/utils/`**: System utilities and portability layer.
- **`objects/`**: Compiled object files (`.o`), kept separate from source to maintain a clean tree.
- **`Makefile`**: Dynamic build system linking against `zlib` (`-lz`) and `libcurl` (`-lcurl`).

### Core Modules
- **`main.c`**: CLI entry point and command dispatcher.
- **`src/ui/ui.c`**: Centralized UI module using GitHub Primer design system colors (Truecolor).
- **`src/core/object/`**: Content-addressable storage (Blobs, Trees, Commits).
- **`src/core/index/`**: Binary staging area management.
- **`src/core/hash/`**: SHA-256 implementation.
- **`src/core/config/`**: Repository and global configuration.
- **`src/core/refs/`**: Branch and HEAD management.

## Technical Implementation Details

### Data Storage & Integrity
- **Objects**: Compressed blobs stored at `.cit/objects/[first-2-chars-sha]/[rest-of-sha]`.
- **Refs**: Branch pointers at `.cit/refs/heads/[branch-name]`.
- **HEAD**: Pointer to current branch at `.cit/HEAD`.
- **Index**: Binary file at `.cit/index` containing `IndexEntry` structs.
- **SHA-256**: All content is identified by its SHA-256 hash.

### User Interface (GitHub.com Aesthetic)
- **Colors**: Uses exact HEX tokens from GitHub's Primer design system via Truecolor ANSI.
  - `FG_SUCCESS`: `#3FB950` (Green)
  - `FG_DANGER`: `#F85149` (Red)
  - `FG_ATTENTION`: `#D29922` (Yellow)
  - `FG_ACCENT`: `#58A6FF` (Blue)
- **Output**: Structured and user-friendly with consistent symbols and headers.

## CLI Reference
(See README.md for full command list)

--- Changes Made ---
- **Refactoring Stage 1: Modularization**
  - Created modular directory structure in `src/`: `core/object`, `core/index`, `core/hash`, `core/config`, `core/refs`, `commands`, `utils`, `ui`.
  - Moved `utils.c`, `utils.h`, and `portability.h` to `src/utils/`.
  - Moved `sha256.c` and `sha256.h` to `src/core/hash/`.
  - Updated `Makefile` to dynamically find source files and include all modular directories. Added `mkdir -p` logic for all object subdirectories.
  - Split `commands.c` into individual files in `src/commands/`: `cmd_init.c`, `cmd_add.c`, `cmd_commit.c`, `cmd_status.c`, `cmd_log.c`, `cmd_branch.c`, `cmd_config.c`, `cmd_checkout.c`.
  - Created `src/core/config/config.c/h` and moved `check_config` logic there.
  - Created `src/core/refs/refs.c/h` and moved branch/HEAD management logic there.
  - Refined all commands to use modularized core logic.
- **UI & Aesthetic Overhaul (GitHub-themed)**
  - Implemented `src/ui/ui.c/h` with Truecolor ANSI support and GitHub Primer color palette.
  - Updated `cmd_init`, `cmd_status`, `cmd_log`, `cmd_branch`, and `cmd_commit` to use the new UI module for a professional, structured output.
- **Feature Addition: Diff Command**
  - Implemented Myers Diff algorithm in `src/utils/diff.c/h`.
  - Added `cit diff` command in `src/commands/cmd_diff.c` to show unstaged changes (Working Tree vs Index).
  - Integrated `diff` into `main.c` and `commands.h`.
- **Feature Addition: Show Command**
  - Added `cit show <sha>` in `src/commands/cmd_show.c` to display commit details and message.
- **Feature Addition: Reset Command**
  - Added `cit reset <file>` in `src/commands/cmd_reset.c` to unstage files from the index.
- **Feature Addition: Clone Command**
  - Added `cit clone <url>` in `src/commands/cmd_clone.c`.
  - Uses system `git clone` for protocol handling and then converts/initializes a `cit` repository.
  - Prompts user to delete the original `.git` folder after conversion.
