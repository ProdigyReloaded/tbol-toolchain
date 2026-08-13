# TBOL Tools

Tools for working with TBOL (TRINTEX Basic Object Language), the programming language used by the Prodigy online service (1984-1999).

## Components

### Compiler (`compiler/`)

A modern C implementation of the TBOL compiler, producing bytecode `.COD` files from TBOL source.

```bash
./tbolc [options] <source.src>

Options:
  -o <dir>              Output directory (default: current directory)
  -I <path>             Add search path for COPY files (can repeat)
  --check               Check only, don't emit code
  -E                    Preprocess only
  --compat              Use original TBOL 4.21 error message format
  --if-goto-opt         Enable IF-THEN-GOTO single-jump optimization
  --dump-tokens         Dump lexer tokens
  --dump-ast            Dump parse tree
  --version             Print version and exit
  --help                Print this help and exit
```

### Decompiler (`decompiler/`)

Converts compiled `.COD` bytecode back into human-readable TBOL source with
recovered expressions and flow control.

Output is verified by recompilation: the decompiler iteratively refines its output until
the recompiled bytecode is identical to the original.

The TBOL decompiler is a work in progress with several known issues.  If it succeeds,
one can trust the decompiler produced something that the compiler will re-compile to
identical bytecode.  But many valid high-level representations reduce to the same bytecode,
so review the output before trusting it as a faithful source.

Notably, it does not reliably recognize ELSE IF chains, which it therefore may emit
very verbosely.

Also, it cannot reliably determine the usage of an RDA variable as an array or a
structure, and this is a common reason why the decompiler may report failure.  A
user can choose to force decompilation and then diff the byte code provided to and
produced by the decompiler.  From this diff, the user will see the few bytes which
differ and experimentally alter the RDA `DATA` section to find a fitting source
code that will round-trip.

```bash
./tboldc [options] <input.cod>

Options:
  -I <path>        Include path for XXCGTSYS (GEV name resolution)
  -o <file>        Output file (default: stdout)
  -n               No verification (skip round-trip refinement)
  -f               Force output even if verification fails
  -m <N>           Max verification iterations (default: 64)
  --no-format      Skip the formatter pass on emitted source
```

### Disassembler (`disassembler/`)

Converts compiled `.COD` bytecode back into TBOL source but without any expression
or control flow recovery.  Instead of IF/THEN, WHILE/THEN, and compound expressions,
one will find the expressions reduced and tested with low-level conditional branch
instructions.

The output of the disassembler. cannot be directly compiled.

```bash
./tboldasm <input.cod>
```

### Formatter (`formatter/`)

Both a library for canonical TBOL formatting (consumed by the decompiler and
language server) and a `tbolfmt` CLI that reads source on stdin and writes the
formatted result on stdout.

```bash
./tbolfmt < input.src > formatted.src
```

### Language Server (`lsp/`)

An LSP server providing IDE features for TBOL:

- Hover info, go-to-definition, find references
- Completions (variables, registers, procedures, verbs)
- Document symbols and workspace symbols
- Rename support (including cross-file COPY cascading)
- Signature help for TBOL verbs
- Folding ranges, semantic tokens, selection ranges
- Diagnostics (syntax errors, undefined variables)

### VS Code Extension (`vscode-extension/`)

Full-featured VS Code integration including syntax highlighting, all LSP
features above, build tasks (Ctrl+Shift+B), bytecode decompilation, and
raw disassembly.  The `.vsix` bundles only the language server; install
the toolchain (`tbol-tools-*.tar.gz`/`.zip` from a GitHub Release) and
either put the binaries on `$PATH` or configure `tbol.compilerPath`,
`tbol.decompilerPath`, and `tbol.disassemblerPath` to use the compile,
decompile, and disassemble features.

See [`vscode-extension/README.md`](vscode-extension/README.md) for
configuration details. Quick build:

```bash
cd vscode-extension
make all      # Build extension and bundle the language server
make package  # Produce a .vsix
make install  # Install into your VS Code
```

Requires Node.js 18+ and `npm`.

### Shared Bytecode Library (`shared/`)

Core infrastructure for bytecode I/O, instruction decoding, and program
analysis.  Used by the compiler, decompiler, disassembler, and language server.

## Building

Requires: flex, bison, and a C11-compatible compiler.

```bash
cmake --preset default     # Debug build into build/
cmake --build build
```

`CMakePresets.json` defines three presets:

| Preset | Build dir | Build type | Notes |
|---|---|---|---|
| `default` | `build/` | Debug | Day-to-day development. |
| `release` | `build-release/` | Release | Optimized binaries. |
| `asan` | `build-asan/` | Debug | Adds `-fsanitize=address,undefined` for sanitizer runs. |

After a successful build, the binaries land at:

```
build/compiler/tbolc
build/decompiler/tboldc
build/disassembler/tboldasm
build/formatter/tbolfmt
build/lsp/tbol-lsp
```

Substitute `build-release` or `build-asan` when using those presets.

## Installing

Each component's `CMakeLists.txt` declares an install target (`DESTINATION
bin`). To install into a chosen prefix:

```bash
cmake --install build --prefix /usr/local
```

Without `--prefix`, the install lands at `CMAKE_INSTALL_PREFIX`, which the
`default` preset sets to the directory above the source tree.

## Packaging

To produce a distributable binary archive of the toolchain:

```bash
cmake --preset release
cmake --build build-release --target package
```

This invokes CPack and writes platform-tagged archives into `build-release/`:

```
build-release/tbol-tools-<version>-<os>-<arch>.tar.gz   (Linux, macOS)
build-release/tbol-tools-<version>-<os>-<arch>.zip      (all platforms)
```

Each archive contains a top-level `tbol-tools-<version>-<os>-<arch>/`
directory holding `bin/{tbolc,tboldc,tboldasm,tbolfmt,tbol-lsp}`.

A full release combines the toolchain archive (one per OS/arch) with the
platform-independent `.vsix` produced by `cd vscode-extension && make
package`.

## Testing

```bash
ctest --test-dir build --output-on-failure
```

The ctest suite drives five per-component test runners:

| Suite | Source |
|---|---|
| `compiler_tests` | `compiler/tests/run_tests.sh` (positive corpus + golden output) |
| `decompiler_tests` | `decompiler/tests/run_tests.sh` (round-trip refinement) |
| `disassembler_tests` | `disassembler/tests/run_tests.sh` (vs `tests/reference/*.dasm`) |
| `formatter_tests` | `formatter/tests/run_tests.sh` (idempotent format check) |
| `lsp_tests` | `lsp/test-lsp` (document-store and parser regression suite) |

The compiler's positive corpus at `compiler/tests/positive/` (451 `.src`
programs, each with a golden `.cod`) is the primary corpus; the disassembler
and decompiler suites re-read those same golden `.cod` files. The per-verb
round-trip corpus at `tests/roundtrip/` (68 `.src` programs) and the focused
fixtures at `tests/COMPLEX*.SRC`, `tests/GEVTEST.SRC`, `tests/MSZX010X.src`,
`tests/clear_array_bug.SRC` are exercised by the component suites above.

To build with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake --preset asan
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

## Memory checking (Valgrind)

`Dockerfile.valgrind` provides a self-contained Ubuntu image that builds the
toolchain and runs the full ctest suite under valgrind (`ctest -T memcheck`,
`--leak-check=full`, definite+possible leaks treated as errors).

```bash
docker build -f Dockerfile.valgrind -t tbol-valgrind .
docker run --rm tbol-valgrind
```

The container prints a summary at the end (`ERROR SUMMARY`, `definitely lost`,
`indirectly lost`); the run exits non-zero on any leak.

`Dockerfile.valgrind.supp` suppresses two upstream leaks in `/usr/bin/bash`
(Ubuntu 24.04 ships bash 5.2) that valgrind reports once per subshell invoked
by the test scripts: 17 bytes from `redirection_expand` and 24 bytes from
`strvec_from_word_list`. Wrapping each TBOL binary directly under valgrind
shows them clean (`definitely lost: 0 bytes` for `tbolc`, `tboldc`,
`tboldasm`, `tbolfmt`, and the LSP test harness); the suppressions cover only
those two bash signatures, so leaks introduced by TBOL code will still surface.

## TBOL Language Overview

TBOL is a structured programming language with:

- **Registers**: Integer (I1-I8), Decimal (D1-D8), Pointer (P0-P8)
- **RDA**: Runtime Data Area slots for string storage (RDA0-RDA221)
- **GEVs**: Global Environment Variables for system state (#1-#32000)
- **PEVs**: Page Environment Variables for page-local data (&1-&256)
- **Control flow**: IF/THEN/ELSE, WHILE/THEN, DO/END
- **Boolean logic**: AND, OR (no NOT operator)

**Example:**
```tbol
PROGRAM HELLO;

COPY XXCGTSYS;

DATA vars = MESSAGE, RESULT;

PROC main =
    MOVE 'Hello, Prodigy!', MESSAGE;
    IF SYS_RETURN_CODE = '0' THEN DO
        MOVE '1', RESULT;
    END;
    EXIT RESULT;
END_PROC
```

More complete reference documentation about the language is under development.

## About TBOL

TBOL (TRINTEX Basic Object Language) was created by TRINTEX, a joint venture of IBM and
Sears that developed and operated the Prodigy online service (1984-1999).

TBOL was interpreted by a component in the original Reception System (client) named
`API`.  In later versions, a second proprietary language PAL was introduced.

This project is an independent reimplementation of TBOL development tools for
historical preservation and research. It is not affiliated with or endorsed by
the original creators.

"Prodigy" and "TRINTEX" are trademarks of their respective owners.

## License

Copyright 2025-2026, Phillip Heller

This project is part of [Prodigy Reloaded](https://github.com/ProdigyReloaded) and is
licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.
