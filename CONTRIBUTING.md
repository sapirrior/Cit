# Contributing to Cit

Thank you for your interest in contributing to Cit! We value professional, clear, and high-quality contributions that align with our minimalist and local-first philosophy.

To maintain the integrity and professionalism of the project, all contributors are expected to follow these guidelines.

---

## 1. Professional Tone and Technical Accuracy

Cit is an engineering-focused project. We prioritize technical clarity over marketing language.

### Avoid Hyperbole and "Fluff"
Do **not** use subjective, hyperbolic, or superlative language anywhere in the codebase, documentation, or commit messages. Examples of prohibited words and phrases:
- "High-performance" (unless backed by specific benchmarks in the context)
- "Best", "World-class", "State-of-the-art"
- "Fastest", "Incredible", "Amazing"
- "Revolutionary", "Game-changing"

**Rule:** Stick to facts. Describe *what* the code does, not how "good" it is.
- **Bad:** "Implement an amazing high-performance hashing algorithm."
- **Good:** "Implement SHA-256 hashing for object identification."

### Conciseness
Avoid "fluff" or conversational filler in technical documentation and code comments. Be direct and concise.

---

## 2. Commit Message Guidelines

We follow a strict, structured commit message format based on the [Conventional Commits](https://www.conventionalcommits.org/) specification. This allows for automated changelog generation and clear project history.

### Format
```text
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

### Types
- `feat`: A new feature
- `fix`: A bug fix
- `docs`: Documentation only changes
- `style`: Changes that do not affect the meaning of the code (white-space, formatting, etc.)
- `refactor`: A code change that neither fixes a bug nor adds a feature
- `perf`: A code change that improves performance (must include objective reasoning)
- `test`: Adding missing tests or correcting existing tests
- `build`: Changes that affect the build system or external dependencies
- `ci`: Changes to our CI configuration files and scripts
- `chore`: Other changes that don't modify src or test files

### Rules for the Description
- Use the **imperative, present tense**: "change" not "changed" nor "changes".
- Do not capitalize the first letter.
- No dot (`.`) at the end.
- Be specific but brief.

**Example:**
`feat(index): add support for recursive directory staging`

---

## 3. Pull Request Process

1.  **Issue First:** For significant features or changes, please open an issue first to discuss the proposal.
2.  **Branching:** Create a descriptive branch name (e.g., `fix/sha-buffer-overflow` or `feat/cmd-status-verbose`).
3.  **Code Standards:**
    - **Naming Conventions**:
        - **Functions**: Use `snake_case` (e.g., `read_index`, `write_object`).
        - **Variables**: Use `snake_case` (e.g., `current_sha256`, `path_buffer`).
        - **Structs**: Use `PascalCase` (e.g., `IndexEntry`, `TreeStack`).
        - **Macros & Enums**: Use `SCREAMING_SNAKE_CASE` (e.g., `MAX_STACK`, `OBJ_BLOB`).
    - **Path Safety**: Use `normalize_path()` from `src/utils/utils.h` for all user-provided paths.
    - **Merkle Trees**: New tree-aware commands must support recursive traversal of the hierarchical tree object format (`<type> <SHA> <name>\n`).
    - Adhere to the existing C99+ coding style.
    - Ensure `make clean && make` passes without warnings.
4.  **Documentation:** Update `README.md` or `GEMINI.md` if your change affects the user-facing API or build process.
5.  **Review:** All PRs require at least one review from a maintainer. Address all feedback promptly.

---

## 4. Reporting Bugs

When reporting a bug, please include:
- A clear, factual description of the issue (no hyperbole).
- Steps to reproduce.
- Expected vs. actual behavior.
- System information (OS, compiler version, zlib/libcurl versions).

---

## 5. Licensing

By contributing to Cit, you agree that your contributions will be licensed under the project's [MIT License](LICENSE).
