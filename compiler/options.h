/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 */
/*
 * TBOL Compiler - Command Line Options
 */

#ifndef TBOLC_OPTIONS_H
#define TBOLC_OPTIONS_H

#include <stdbool.h>

#define MAX_INCLUDE_PATHS 32

typedef enum {
    DIAG_FMT_TEXT,      /* Modern clang-style */
    DIAG_FMT_COMPAT,    /* Original TBOL 4.21 format */
    DIAG_FMT_JSON,      /* JSON for tooling */
} DiagFormat;

typedef struct {
    /* Input */
    const char *input_file;

    /* Output */
    const char *output_dir;     /* -o: output directory (NULL = current) */
    bool emit_sym;              /* --sym: emit .SYM file */
    bool emit_sym_json;         /* --sym-json: emit .SYM.json file */
    bool emit_lst;              /* --lst: emit .LST file */
    bool check_only;            /* --check: syntax check, no output */
    bool preprocess_only;       /* -E: preprocess only, output to stdout */

    /* Include paths */
    const char *include_paths[MAX_INCLUDE_PATHS];
    int include_path_count;

    /* Compatibility */
    bool compat_errors;         /* --compat: original error format */
    bool strict_mode;           /* --strict: match original quirks */
    bool if_goto_opt;           /* --if-goto-opt: enable IF-THEN-GOTO optimization (off by default for original compiler equivalence) */

    /* Diagnostics */
    DiagFormat diag_format;     /* --diagnostics-format */
    bool warnings_as_errors;    /* -Werror */
    bool warn_redefine;         /* Warn on DEFINE redefinition (default true) */

    /* Debug */
    bool dump_tokens;           /* --dump-tokens */
    bool dump_ast;              /* --dump-ast */
    bool verbose;               /* --verbose */

    /* Info flags */
    bool show_version;
    bool show_help;
} Options;

/* Global options instance */
extern Options g_options;

/* Parse command line arguments */
int options_parse(int argc, char **argv);

/* Print usage/help */
void options_print_usage(const char *program_name);

/* Print version */
void options_print_version(void);

#endif /* TBOLC_OPTIONS_H */
