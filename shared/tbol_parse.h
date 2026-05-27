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
 * TBOL Parser Interface
 *
 * Library interface for parsing TBOL source code.
 * Used by the LSP server and other tools that need to parse TBOL.
 */
#ifndef TBOL_PARSE_H
#define TBOL_PARSE_H

#include "ast.h"
#include "../compiler/lexer/preproc_event.h"
#include <stdbool.h>

/*
 * Parse result with diagnostics
 */
typedef enum {
    TBOL_DIAG_ERROR,
    TBOL_DIAG_WARNING,
    TBOL_DIAG_NOTE,
} TbolDiagLevel;

typedef struct {
    TbolDiagLevel level;
    char *message;
    char *filename;
    int line;
    int column;
    int end_column;     /* For highlighting range */
} TbolDiagnostic;

typedef struct {
    /* Parse result */
    AstNode *ast;           /* Root AST node (caller must free with ast_free) */
    bool success;           /* True if parse succeeded without errors */

    /* Diagnostics */
    TbolDiagnostic *diagnostics;
    int diagnostic_count;
    int error_count;
    int warning_count;

    /* Preprocessor events (COPY/DEFINE references for LSP) */
    PreprocEvent *preproc_events;
    int preproc_event_count;

} TbolParseResult;

/*
 * Parse options
 */
typedef struct {
    /* Include paths for COPY resolution */
    const char **include_paths;
    int include_path_count;

    /* Filename for error messages (can be "untitled" for unsaved) */
    const char *filename;

    /* Parse mode */
    bool check_only;        /* Don't do semantic analysis */
    bool collect_symbols;   /* Populate symbol information */

    /* Content overrides for COPY files (LSP dirty buffer injection) */
    const char **override_paths;
    const char **override_contents;
    int override_count;
} TbolParseOptions;

/*
 * Parse TBOL source code from a string
 *
 * The source string is copied internally, caller retains ownership.
 * Caller must free the result with tbol_parse_result_free().
 */
TbolParseResult *tbol_parse_string(const char *source, const TbolParseOptions *options);

/*
 * Parse TBOL source code from a file
 *
 * Caller must free the result with tbol_parse_result_free().
 */
TbolParseResult *tbol_parse_file(const char *filename, const TbolParseOptions *options);

/*
 * Free a parse result
 */
void tbol_parse_result_free(TbolParseResult *result);

#endif /* TBOL_PARSE_H */
