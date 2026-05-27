/*
 * compile.c — TBOL compiler library interface
 *
 * Encapsulates the full compilation pipeline (lex → parse → sema → codegen)
 * into a single callable function. Used by the decompiler for round-trip
 * verification without forking a separate process.
 */

#include "compile.h"
#include "options.h"
#include "../shared/ast.h"
#include "diag/diag.h"
#include "lexer/preproc.h"
#include "sema/sema.h"
#include "sema/symtab.h"
#include "codegen/codegen.h"
#include "parser/tbol.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

/* Flex/Bison globals */
extern FILE *yyin;
extern int yyparse(void);
extern AstNode *ast_root;
extern void lexer_set_filename(const char *filename);
extern int yylex_destroy(void);
extern void yyrestart(FILE *input_file);

int tbolc_compile_file(const char *src_path, const char *out_dir,
                        const char **include_paths, int include_count) {
    /* Set up options for this compilation */
    memset(&g_options, 0, sizeof(g_options));
    g_options.input_file = src_path;
    g_options.output_dir = out_dir;
    g_options.warn_redefine = true;
    for (int i = 0; i < include_count && i < MAX_INCLUDE_PATHS; i++)
        g_options.include_paths[g_options.include_path_count++] = include_paths[i];

    /* Initialize subsystems */
    diag_init();
    preproc_init();
    ast_root = NULL;

    /* Set up preprocessor with source file directory */
    char *filename_copy = strdup(src_path);
    char *dir = dirname(filename_copy);
    preproc_set_current_dir(dir);
    free(filename_copy);

    /* Open and compile */
    FILE *fp = fopen(src_path, "r");
    if (!fp) {
        preproc_cleanup();
        diag_cleanup();
        return 1;
    }

    yyin = fp;
    yyrestart(fp);
    lexer_set_filename(src_path);

    int result = yyparse();
    fclose(fp);

    if (result != 0 || diag_has_errors()) {
        goto cleanup;
    }

    /* Semantic analysis */
    if (sema_analyze(ast_root) != 0) {
        result = 1;
        goto cleanup;
    }

    /* Extract base name for output file */
    char *input_copy = strdup(src_path);
    char *base = basename(input_copy);
    char *dot = strrchr(base, '.');
    if (dot) *dot = '\0';
    char *base_name = strdup(base);
    free(input_copy);

    /* Code generation */
    if (codegen_generate(ast_root, out_dir, base_name) != 0) {
        result = 1;
    }
    free(base_name);

cleanup:
    if (ast_root) {
        ast_free(ast_root);
        ast_root = NULL;
    }
    symtab_cleanup();
    preproc_cleanup();
    /* Don't call yylex_destroy — it breaks subsequent yyrestart calls.
     * The scanner state is reset by yyrestart at the start of each compile. */
    diag_cleanup();

    return result;
}

AstNode *tbolc_parse_file(const char *src_path,
                           const char **include_paths, int include_count) {
    memset(&g_options, 0, sizeof(g_options));
    g_options.input_file = src_path;
    g_options.warn_redefine = true;
    for (int i = 0; i < include_count && i < MAX_INCLUDE_PATHS; i++)
        g_options.include_paths[g_options.include_path_count++] = include_paths[i];

    diag_init();
    preproc_init();
    ast_root = NULL;

    char *filename_copy = strdup(src_path);
    char *dir = dirname(filename_copy);
    preproc_set_current_dir(dir);
    free(filename_copy);

    FILE *fp = fopen(src_path, "r");
    if (!fp) {
        preproc_cleanup();
        diag_cleanup();
        return NULL;
    }

    yyin = fp;
    yyrestart(fp);
    lexer_set_filename(src_path);

    int result = yyparse();
    fclose(fp);

    AstNode *root = NULL;
    if (result == 0 && !diag_has_errors() && ast_root) {
        root = ast_root;
        ast_root = NULL;  /* Transfer ownership to caller */
    }

    if (ast_root) { ast_free(ast_root); ast_root = NULL; }
    symtab_cleanup();
    preproc_cleanup();
    diag_cleanup();

    return root;
}
