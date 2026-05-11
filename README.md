<p align="center">
  <img src="assets/cit-round.png" alt="Cit Logo" width="100">
</p>

# Cit - Local-First Version Control

Cit is a high-integrity version control system written in C. It is designed to provide a secure and fast local version control experience by employing SHA-256 content addressing, hierarchical Merkle trees, and advanced storage optimizations.

## Core Technical Specifications
- **Data Integrity**: Mandatory SHA-256 identification for all objects (blobs, trees, and commits).
- **Architecture**: Hierarchical Merkle tree structure for efficient directory-level hashing and state management.
- **Storage Optimization**: Proprietary Cit-LZ compression (LZSS + Range Coder) and recursive delta encoding for minimal storage footprint.
- **Security**: Built-in path normalization and transactional object creation with automatic rollback on failure.
- **Portability**: POSIX-compliant implementation supporting Linux, macOS, Android (Termux), and Windows environments.

## Installation and System Integration

### 1. Build from Source
To compile Cit, ensure that the GCC compiler, Make, and the zlib development library are installed on your system. Execute the following command in the project root:
```bash
make
```

### 2. Global Installation (Recommended)
To use Cit efficiently from any directory, move the compiled binary to a location in your system's execution path (PATH).

For Linux, macOS, or Termux:
```bash
# Move the binary to a standard local bin directory
mkdir -p ~/bin
cp cit ~/bin/

# Ensure ~/bin is in your shell's PATH by adding this to your .bashrc or .zshrc:
export PATH="$HOME/bin:$PATH"
```

Alternatively, for system-wide access (requires administrative privileges):
```bash
sudo cp cit /usr/local/bin/
```

## Initial Configuration
You must configure your global identity before performing commit or branch operations:
```bash
cit config -u "Full Name"
cit config -e "email@example.com"
```

## Basic Operations Guide

### Repository Initialization
Convert a directory into a Cit repository:
```bash
cit init
```

### Staging Changes
Add files or directories to the staging area:
```bash
cit add <filename>    # Stage a specific file
cit add .             # Stage all changes in the current directory
```

### Status Reporting
View the current state of the working directory and staged changes:
```bash
cit status
```

### Committing Changes
Record the staged snapshot into the repository history:
```bash
cit commit "Descriptive commit message"
```

### History and Branching
- View commit logs: `cit log`
- List all branches: `cit branch`
- Create a new branch: `cit branch <branch_name>`
- Delete a branch: `cit branch -d <branch_name>`

### State Restoration
Restore the working directory or specific files to a previous state:
```bash
cit checkout <commit_sha>
```
Note: Cit will request manual confirmation (y/n) before overwriting existing local files.

## License
This project is licensed under the MIT License.
