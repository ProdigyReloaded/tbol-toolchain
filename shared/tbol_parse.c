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
 * TBOL Parser Interface Implementation
 */
#include "tbol_parse.h"
#include "../compiler/diag/diag.h"
#include "../compiler/lexer/preproc.h"
#include "../compiler/lexer/preproc_event.h"
#include "../compiler/sema/sema.h"
#include "../compiler/sema/symtab.h"
#include "../compiler/parser/tbol.tab.h"
#include "../compiler/options.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>  /* dirname */

/* External declarations from lexer/parser */
extern FILE *yyin;
extern int yyparse(void);
extern void yyrestart(FILE *input_file);
extern AstNode *ast_root;
extern void lexer_set_filename(const char *filename);

/* For parsing from string - we use fmemopen or a temp file */
#ifdef __APPLE__
/* macOS has fmemopen in newer versions but we'll use a portable approach */
#define USE_TMPFILE 1
#else
#define USE_TMPFILE 0
#endif

/*
 * Convert internal diagnostics to result format
 */
static void collect_diagnostics(TbolParseResult *result) {
    result->diagnostic_count = g_diag.count;
    result->error_count = g_diag.error_count;
    result->warning_count = g_diag.warning_count;

    if (g_diag.count == 0) {
        result->diagnostics = NULL;
        return;
    }

    result->diagnostics = calloc(g_diag.count, sizeof(TbolDiagnostic));

    for (int i = 0; i < g_diag.count; i++) {
        Diagnostic *src = &g_diag.diagnostics[i];
        TbolDiagnostic *dst = &result->diagnostics[i];

        switch (src->level) {
            case DIAG_ERROR:   dst->level = TBOL_DIAG_ERROR; break;
            case DIAG_WARNING: dst->level = TBOL_DIAG_WARNING; break;
            case DIAG_NOTE:    dst->level = TBOL_DIAG_NOTE; break;
        }

        dst->message = src->message ? strdup(src->message) : NULL;
        dst->filename = src->loc.filename ? strdup(src->loc.filename) : NULL;
        dst->line = src->loc.line;
        dst->column = src->loc.column;
        dst->end_column = src->loc.column + src->highlight_len;
    }
}

/*
 * Parse from string
 */
TbolParseResult *tbol_parse_string(const char *source, const TbolParseOptions *options) {
    TbolParseResult *result = calloc(1, sizeof(TbolParseResult));

    /* Initialize compiler options to safe defaults */
    memset(&g_options, 0, sizeof(g_options));

    /* Initialize compiler infrastructure */
    diag_init();
    preproc_init();
    symtab_init();

    /* Set up include paths */
    if (options && options->include_paths) {
        for (int i = 0; i < options->include_path_count; i++) {
            preproc_add_include_path(options->include_paths[i]);
        }
    }

    /* Set current directory from filename for COPY resolution */
    if (options && options->filename) {
        char *path_copy = strdup(options->filename);
        char *dir = dirname(path_copy);
        if (dir && strcmp(dir, ".") != 0) {
            preproc_set_current_dir(dir);
        }
        free(path_copy);
    }

    /* Create temporary file for source */
    FILE *fp = tmpfile();
    if (!fp) {
        TbolDiagnostic *diag = calloc(1, sizeof(TbolDiagnostic));
        diag->level = TBOL_DIAG_ERROR;
        diag->message = strdup("Failed to create temporary file for parsing");
        diag->line = 1;
        diag->column = 1;
        result->diagnostics = diag;
        result->diagnostic_count = 1;
        result->error_count = 1;
        result->success = false;
        return result;
    }

    /* Write source to temp file */
    size_t len = strlen(source);
    fwrite(source, 1, len, fp);
    rewind(fp);

    /* Set up lexer - must use yyrestart to properly reset scanner state */
    yyrestart(fp);
    const char *filename = (options && options->filename) ? options->filename : "<string>";
    lexer_set_filename(filename);

    /* Free any leftover AST from a previous parse (e.g., a prior error parse
     * where the PROGRAM node was partially built but not claimed) */
    if (ast_root) {
        ast_free(ast_root);
        ast_root = NULL;
    }

    /* Register content overrides for COPY files (LSP dirty buffers) */
    if (options && options->override_count > 0) {
        for (int i = 0; i < options->override_count; i++) {
            preproc_set_content_override(options->override_paths[i],
                                          options->override_contents[i]);
        }
    }

    /* Set up preprocessor event collection (LSP mode only) */
    PreprocEventList event_list;
    if (options && options->collect_symbols) {
        preproc_event_init(&event_list);
        g_preproc_events = &event_list;
    }

    /* Parse */
    int parse_result = yyparse();
    fclose(fp);

    /* Transfer preprocessor events */
    if (options && options->collect_symbols) {
        g_preproc_events = NULL;
        result->preproc_events = event_list.events;
        result->preproc_event_count = event_list.count;
        event_list.events = NULL;  /* Ownership transferred */
        event_list.count = 0;
    }

    if (parse_result != 0 || diag_has_errors()) {
        result->success = false;
    } else {
        result->success = true;
    }

    /* Semantic analysis (unless check_only) */
    if (result->success && ast_root && !(options && options->check_only)) {
        if (sema_analyze(ast_root) != 0) {
            result->success = false;
        }
    }

    /* Collect diagnostics */
    collect_diagnostics(result);

    /* Transfer AST ownership */
    result->ast = ast_root;
    ast_root = NULL;

    /* Clear content overrides */
    preproc_clear_content_overrides();

    /* Always clean up symbol table. Previous versions skipped this when
     * collect_symbols was set, but that leaked structure groups and variable
     * entries on every reparse. The LSP reads symbol info from the AST and
     * preproc events, not from the global symbol table. */
    symtab_cleanup();
    preproc_cleanup();
    lexer_clear_filename_pool();  /* Free interned filename strings - AST owns copies */
    diag_cleanup();

    return result;
}

/*
 * Parse from file
 */
TbolParseResult *tbol_parse_file(const char *filename, const TbolParseOptions *options) {
    /* Read file into string */
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        TbolParseResult *result = calloc(1, sizeof(TbolParseResult));
        TbolDiagnostic *diag = calloc(1, sizeof(TbolDiagnostic));
        diag->level = TBOL_DIAG_ERROR;
        diag->message = malloc(256);
        snprintf(diag->message, 256, "Cannot open file: %s", filename);
        diag->filename = strdup(filename);
        diag->line = 1;
        diag->column = 1;
        result->diagnostics = diag;
        result->diagnostic_count = 1;
        result->error_count = 1;
        result->success = false;
        return result;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        TbolParseResult *err = calloc(1, sizeof(TbolParseResult));
        err->success = false;
        return err;
    }
    fseek(fp, 0, SEEK_SET);

    /* Read content */
    char *content = malloc(size + 1);
    size_t read = fread(content, 1, size, fp);
    content[read] = '\0';
    fclose(fp);

    /* Create options with filename if not provided */
    TbolParseOptions opts = {0};
    if (options) {
        opts = *options;
    }
    if (!opts.filename) {
        opts.filename = filename;
    }

    /* Parse string */
    TbolParseResult *result = tbol_parse_string(content, &opts);

    free(content);
    return result;
}

/*
 * Free parse result
 */
void tbol_parse_result_free(TbolParseResult *result) {
    if (!result) return;

    if (result->ast) {
        ast_free(result->ast);
    }

    for (int i = 0; i < result->diagnostic_count; i++) {
        free(result->diagnostics[i].message);
        free(result->diagnostics[i].filename);
    }
    free(result->diagnostics);

    /* Free preprocessor events */
    if (result->preproc_events) {
        PreprocEventList tmp = {
            .events = result->preproc_events,
            .count = result->preproc_event_count,
            .capacity = result->preproc_event_count
        };
        preproc_event_cleanup(&tmp);
    }

    free(result);
}

