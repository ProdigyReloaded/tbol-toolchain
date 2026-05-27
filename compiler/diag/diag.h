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
 * TBOL Compiler - Diagnostic Infrastructure
 */

#ifndef TBOLC_DIAG_H
#define TBOLC_DIAG_H

#include "../../shared/ast.h"
#include <stdbool.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
} DiagLevel;

typedef struct {
    DiagLevel level;
    SourceLoc loc;
    char *message;
    char *source_line;      /* Source line for context (modern format) */
    int highlight_start;    /* Column to start highlight */
    int highlight_len;      /* Length of highlight */
} Diagnostic;

typedef struct {
    Diagnostic *diagnostics;
    int count;
    int capacity;
    int error_count;
    int warning_count;
} DiagContext;

/* Global diagnostic context */
extern DiagContext g_diag;

/* Initialize/cleanup */
void diag_init(void);
void diag_cleanup(void);

/* Report diagnostics */
void diag_error(SourceLoc loc, const char *fmt, ...);
void diag_warning(SourceLoc loc, const char *fmt, ...);

/* Store source line for context (called by lexer) */
void diag_set_source_line(const char *filename, int line, const char *text);

/* Print all diagnostics */
void diag_print_all(void);

/* Summary */
void diag_print_summary(void);

/* Check if there were errors */
bool diag_has_errors(void);

/* Get counts */
int diag_error_count(void);
int diag_warning_count(void);

#endif /* TBOLC_DIAG_H */
