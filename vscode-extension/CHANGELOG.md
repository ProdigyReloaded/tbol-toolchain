# Change Log

## 0.1.0 — Initial Release

- Syntax highlighting for TBOL source files (.src) and COPY files
- Language server integration (tbol-lsp) providing:
  - Code completion (keywords, variables, procedures, labels, registers)
  - Hover information for variables, procedures, labels, and DEFINEs
  - Go to Definition
  - Find References
  - Rename Symbol (including cross-file COPY cascading)
  - Document Symbols (outline view)
  - Workspace Symbols
  - Signature Help for TBOL verbs
  - Folding ranges
  - Semantic tokens
  - Selection ranges
- COPY file resolution with configurable include paths
- Diagnostics (syntax errors, undefined variables, type checking)
- `${workspaceFolder}` variable expansion in include paths
