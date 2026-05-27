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
 * TBOL Compiler - Main Entry Point
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "options.h"
#include "../shared/ast.h"
#include "diag/diag.h"
#include "lexer/preproc.h"
#include "sema/sema.h"
#include "sema/symtab.h"
#include "codegen/codegen.h"
#include "parser/tbol.tab.h"

/* External declarations */
extern FILE *yyin;
extern int yyparse(void);
extern int yylex(void);
extern char *yytext;
extern AstNode *ast_root;
extern void lexer_set_filename(const char *filename);

/* Forward declarations */
static int compile(const char *filename);
static int preprocess_only(const char *filename);

int main(int argc, char **argv) {
    /* Parse command line options */
    if (options_parse(argc, argv) != 0) {
        return 1;
    }

    /* Handle info flags */
    if (g_options.show_help) {
        options_print_usage(argv[0]);
        return 0;
    }

    if (g_options.show_version) {
        options_print_version();
        return 0;
    }

    /* Initialize diagnostics and preprocessor */
    diag_init();
    preproc_init();

    /* Preprocess-only or full compile */
    int result;
    if (g_options.preprocess_only) {
        result = preprocess_only(g_options.input_file);
    } else {
        result = compile(g_options.input_file);
    }

    /* Print diagnostics */
    diag_print_all();
    diag_print_summary();

    /* Cleanup */
    if (ast_root) {
        ast_free(ast_root);
        ast_root = NULL;
    }
    symtab_cleanup();
    preproc_cleanup();
    diag_cleanup();

    return result;
}

static int compile(const char *filename) {
    /* Open input file */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return 1;
    }

    /* Set up preprocessor with source file directory */
    char *filename_copy = strdup(filename);
    char *dir = dirname(filename_copy);
    preproc_set_current_dir(dir);
    free(filename_copy);

    /* Set up lexer */
    yyin = fp;
    lexer_set_filename(filename);

    if (g_options.verbose) {
        fprintf(stderr, "Compiling %s...\n", filename);
    }

    /* Parse */
    int parse_result = yyparse();
    fclose(fp);

    if (parse_result != 0 || diag_has_errors()) {
        return 1;
    }

    /* Dump AST if requested */
    if (g_options.dump_ast && ast_root) {
        printf("=== AST ===\n");
        ast_dump(ast_root, 0);
    }

    /* Semantic analysis */
    if (sema_analyze(ast_root) != 0) {
        return 1;
    }

    /* Check-only mode */
    if (g_options.check_only) {
        if (g_options.verbose) {
            fprintf(stderr, "Semantic check passed.\n");
        }
        return 0;
    }

    /* Extract base name from input file */
    char *input_copy = strdup(filename);
    char *base = basename(input_copy);
    /* Strip extension if present */
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    char *base_name = strdup(base);
    free(input_copy);

    /* Code generation */
    if (g_options.verbose) {
        fprintf(stderr, "Generating code...\n");
    }

    if (codegen_generate(ast_root, g_options.output_dir, base_name) != 0) {
        free(base_name);
        return 1;
    }
    free(base_name);

    if (g_options.verbose) {
        fprintf(stderr, "Compilation successful.\n");
    }

    return 0;
}

/*
 * Preprocess only - output expanded source to stdout
 */
static int preprocess_only(const char *filename) {
    /* Open input file */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open '%s'\n", filename);
        return 1;
    }

    /* Set up preprocessor with source file directory */
    char *filename_copy = strdup(filename);
    char *dir = dirname(filename_copy);
    preproc_set_current_dir(dir);
    free(filename_copy);

    /* Set up lexer */
    yyin = fp;
    lexer_set_filename(filename);

    /* Lex all tokens and output their text */
    int token;
    int need_space = 0;
    int last_line = 1;

    while ((token = yylex()) != 0) {
        /* Handle newlines for readability */
        extern int current_line;
        while (last_line < current_line) {
            printf("\n");
            last_line++;
            need_space = 0;
        }

        /* Add space between tokens (except for punctuation) */
        if (need_space && token != ';' && token != ',' && token != ')' && token != ':') {
            printf(" ");
        }

        /* Output token */
        printf("%s", yytext);

        /* Track when we need space */
        need_space = (token != '(' && token != ':');
    }

    printf("\n");
    fclose(fp);

    return diag_has_errors() ? 1 : 0;
}
