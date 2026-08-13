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
 * TBOL Compiler - Command Line Options Implementation
 */

#include "options.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Global options instance */
Options g_options = {0};

#define TBOLC_VERSION "1.0.0"

void options_print_version(void) {
    printf("tbolc %s\n", TBOLC_VERSION);
    printf("TBOL compiler compatible with TBOL 4.21\n");
}

void options_print_usage(const char *program_name) {
    printf("Usage: %s [options] <source.SRC>\n\n", program_name);

    printf("Output Options:\n");
    printf("  -o <dir>              Output directory (default: current directory)\n");
    printf("  --sym                 Emit symbol table (.SYM file)\n");
    printf("  --sym-json            Emit symbol table as JSON (.SYM.json)\n");
    printf("  --lst                 Emit listing file (.LST file)\n");
    printf("  --check               Check only, don't emit code\n");
    printf("  -E                    Preprocess only (shows tokens with DEFINE expanded;\n");
    printf("                        COPY expansion requires full parsing)\n");
    printf("\n");

    printf("Include Paths:\n");
    printf("  -I <path>             Add search path for COPY files (can repeat)\n");
    printf("\n");

    printf("Compatibility:\n");
    printf("  --compat              Use original TBOL 4.21 error message format\n");
    printf("  --strict              Match original compiler quirks exactly\n");
    printf("  --if-goto-opt         Enable IF-THEN-GOTO single-jump optimization\n");
    printf("\n");

    printf("Diagnostics:\n");
    printf("  --diagnostics-format=<fmt>\n");
    printf("                        Error output format: text (default), json\n");
    printf("  -Werror               Treat warnings as errors\n");
    printf("  -Wno-redefine         Suppress warning on DEFINE redefinition\n");
    printf("\n");

    printf("Debugging:\n");
    printf("  --dump-tokens         Dump lexer tokens\n");
    printf("  --dump-ast            Dump parse tree\n");
    printf("  --verbose             Verbose compilation output\n");
    printf("\n");

    printf("Info:\n");
    printf("  --version             Print version and exit\n");
    printf("  --help                Print this help and exit\n");
}

int options_parse(int argc, char **argv) {
    /* Initialize defaults */
    memset(&g_options, 0, sizeof(g_options));
    g_options.diag_format = DIAG_FMT_TEXT;
    g_options.warn_redefine = true;

    for (int i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            g_options.show_help = true;
            return 0;
        }
        else if (strcmp(arg, "--version") == 0) {
            g_options.show_version = true;
            return 0;
        }
        else if (strcmp(arg, "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -o requires an argument\n");
                return -1;
            }
            g_options.output_dir = argv[++i];
        }
        else if (strcmp(arg, "-I") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -I requires an argument\n");
                return -1;
            }
            if (g_options.include_path_count >= MAX_INCLUDE_PATHS) {
                fprintf(stderr, "Error: too many include paths (max %d)\n",
                        MAX_INCLUDE_PATHS);
                return -1;
            }
            g_options.include_paths[g_options.include_path_count++] = argv[++i];
        }
        else if (strncmp(arg, "-I", 2) == 0) {
            /* -Ipath form */
            if (g_options.include_path_count >= MAX_INCLUDE_PATHS) {
                fprintf(stderr, "Error: too many include paths (max %d)\n",
                        MAX_INCLUDE_PATHS);
                return -1;
            }
            g_options.include_paths[g_options.include_path_count++] = arg + 2;
        }
        else if (strcmp(arg, "--sym") == 0) {
            g_options.emit_sym = true;
        }
        else if (strcmp(arg, "--sym-json") == 0) {
            g_options.emit_sym_json = true;
        }
        else if (strcmp(arg, "--lst") == 0) {
            g_options.emit_lst = true;
        }
        else if (strcmp(arg, "--check") == 0) {
            g_options.check_only = true;
        }
        else if (strcmp(arg, "-E") == 0) {
            g_options.preprocess_only = true;
        }
        else if (strcmp(arg, "--compat") == 0) {
            g_options.compat_errors = true;
            g_options.diag_format = DIAG_FMT_COMPAT;
        }
        else if (strcmp(arg, "--strict") == 0) {
            g_options.strict_mode = true;
        }
        else if (strcmp(arg, "--if-goto-opt") == 0) {
            g_options.if_goto_opt = true;
        }
        else if (strcmp(arg, "--no-if-goto-opt") == 0) {
            /* Legacy flag - optimization is now off by default */
        }
        else if (strcmp(arg, "--diagnostics-format=text") == 0) {
            g_options.diag_format = DIAG_FMT_TEXT;
        }
        else if (strcmp(arg, "--diagnostics-format=json") == 0) {
            g_options.diag_format = DIAG_FMT_JSON;
        }
        else if (strcmp(arg, "-Werror") == 0) {
            g_options.warnings_as_errors = true;
        }
        else if (strcmp(arg, "-Wno-redefine") == 0) {
            g_options.warn_redefine = false;
        }
        else if (strcmp(arg, "-Wredefine") == 0) {
            g_options.warn_redefine = true;
        }
        else if (strcmp(arg, "--dump-tokens") == 0) {
            g_options.dump_tokens = true;
        }
        else if (strcmp(arg, "--dump-ast") == 0) {
            g_options.dump_ast = true;
        }
        else if (strcmp(arg, "--verbose") == 0) {
            g_options.verbose = true;
        }
        else if (arg[0] == '-') {
            fprintf(stderr, "Error: unknown option '%s'\n", arg);
            return -1;
        }
        else {
            /* Positional argument - input file */
            if (g_options.input_file) {
                fprintf(stderr, "Error: multiple input files specified\n");
                return -1;
            }
            g_options.input_file = arg;
        }
    }

    /* Validate */
    if (!g_options.show_help && !g_options.show_version) {
        if (!g_options.input_file) {
            fprintf(stderr, "Error: no input file specified\n");
            return -1;
        }
    }

    return 0;
}
