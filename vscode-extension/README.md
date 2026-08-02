# Prodigy TBOL Language Extension for VS Code

Language support for TBOL (TRINTEX Basic Object Language), the programming
language used by the Prodigy online service (1984-1999). Part of the
[Prodigy Reloaded](https://github.com/ProdigyReloaded) project.

## Features

- Syntax highlighting
- Code completion (keywords, variables, procedures, labels)
- Hover information
- Go to Definition
- Find References
- Rename Symbol
- Document Symbols (outline)
- Workspace Symbols
- Signature Help for verbs
- COPY file resolution with configurable include paths
- Diagnostics (syntax errors, undefined variables)
- Disassemble .COD/.PGM bytecode files (command: "TBOL: Disassemble .COD/.PGM File")

## Configuration

### Include Paths

Configure paths where COPY files are located:

```json
{
  "tbol.includePaths": [
    "${workspaceFolder}/includes",
    "${workspaceFolder}/copy"
  ]
}
```

The directory containing each source file is automatically included.

### Tool Paths

By default, the extension searches for bundled tools, sibling directories
(development mode), and the system PATH. To override:

```json
{
  "tbol.serverPath": "/path/to/tbol-lsp",
  "tbol.compilerPath": "/path/to/tbolc",
  "tbol.decompilerPath": "/path/to/tboldc",
  "tbol.disassemblerPath": "/path/to/tboldasm"
}
```

## External tools

The `.vsix` bundles only the language server (`tbol-lsp`); the other
toolchain binaries are not shipped inside it.  Each feature below uses
the binary listed; the extension finds it via the explicit
`tbol.<tool>Path` setting first, then the extension's bundled `bin/`
directory, then a sibling source directory in development mode, then
your shell's `$PATH`.

| Feature | Binary | Setting |
|---|---|---|
| Hover, completions, diagnostics, rename, etc. (LSP) | `tbol-lsp` | bundled |
| `tbolc: compile` build task (Ctrl+Shift+B) | `tbolc` | `tbol.compilerPath` |
| `TBOL: Decompile .COD/.PGM File` | `tboldc` | `tbol.decompilerPath` |
| `TBOL: Disassemble .COD/.PGM File (raw)` | `tboldasm` | `tbol.disassemblerPath` |

Install the TBOL toolchain from the GitHub Releases page
(`tbol-tools-<version>-<os>-<arch>.tar.gz` or `.zip` for your platform).
If you put it somewhere on `$PATH` (`/usr/local/bin`, `~/bin`, etc.),
no configuration is needed.

### Tracing

Enable communication tracing for debugging:

```json
{
  "tbol.trace.server": "verbose"
}
```

## Building from Source

### Prerequisites

- Node.js 18+
- npm
- C compiler (gcc or clang)
- make

### Build

```bash
cd vscode-extension
make all      # Build extension and language server
make package  # Create .vsix package
make install  # Install to VS Code
```

### Development

```bash
make dev      # Build without packaging
```

Then press F5 in VS Code to launch the Extension Development Host.

## About

This extension is part of [Prodigy Reloaded](https://github.com/ProdigyReloaded).
TBOL was created by TRINTEX, a joint venture of IBM and Sears. This project is an
independent reimplementation for historical preservation and research.

## License

GPL-3.0 - See [LICENSE](../LICENSE) for details.
