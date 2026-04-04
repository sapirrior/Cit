# Cit - Modern Local-First Version Control

**Cit** is a high-performance, secure, and lightweight version control system written in C. It follows the core philosophy of Git but upgrades the security to **SHA-256** and optimizes every operation for a lightning-fast **local-first** experience.

## 🚀 Key Features
- **SHA-256 Security**: Mandatory hashing for all objects ensures industry-standard data integrity.
- **Zero-Config Portability**: Works out-of-the-box on Linux, macOS, Android (Termux), and Windows.
- **Visual Status**: Beautifully color-coded status reports to track staged, modified, and untracked files.
- **Memory Efficient**: Built with dynamic resource management to handle projects of any size.

## 🛠️ Installation & Setup

### 1. Build from Source
Ensure you have `gcc`, `make`, and `zlib` installed.
```bash
make
```

### 2. Configure Your Identity
Before committing, set your global name and email:
```bash
./cit config -u "Your Name"
./cit config -e "yourname@example.com"
```

## 📖 Quick Start Guide

### Initialize a Repository
Turn any folder into a Cit repository:
```bash
./cit init
```

### Track and Stage Files
Add files to your staging area (index):
```bash
./cit add my_file.txt    # Add a specific file
./cit add .              # Add everything in the folder
```

### Check Status
See what has changed in your working directory:
```bash
./cit status
```

### Save a Snapshot
Record your staged changes permanently:
```bash
./cit commit "My first commit"
```

### View History
See your project's timeline:
```bash
./cit log
```

### Branching
Work on new features without breaking the main project:
```bash
./cit branch feature-x   # Create a branch
./cit branch             # List all branches
```

### State Restoration
Go back in time or restore specific files:
```bash
./cit checkout <sha>     # Restore the whole project to this commit
```

## ⚠️ Safety First
Cit prioritizes your data. Destructive operations like `checkout` will always ask for your confirmation (`y/n`) before overwriting any of your local files.

## 📄 License
Cit is released under the MIT License. Feel free to use, modify, and distribute!
