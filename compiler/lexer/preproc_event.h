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
 * TBOL Preprocessor Events
 *
 * Records COPY and DEFINE reference events during lexing for use by
 * the LSP (go-to-definition, hover on preprocessor constructs).
 *
 * The global event list pointer (g_preproc_events) is NULL during
 * normal compilation, so there is zero overhead for the compiler.
 */

#ifndef TBOLC_PREPROC_EVENT_H
#define TBOLC_PREPROC_EVENT_H

typedef enum {
    PREPROC_EVENT_COPY,
    PREPROC_EVENT_DEFINE_REF,
} PreprocEventKind;

typedef struct {
    PreprocEventKind kind;
    char *filename;             /* Source file containing the reference */
    int line, column, end_column;  /* 1-based source location */
    char *name;                 /* COPY filename or DEFINE name */
    /* COPY-specific */
    char *resolved_path;        /* Full path to COPY file, or NULL */
    /* DEFINE_REF-specific */
    char *define_value;         /* Expansion value */
    char *def_filename;         /* File where DEFINE was declared */
    int def_line, def_column;
} PreprocEvent;

typedef struct {
    PreprocEvent *events;
    int count;
    int capacity;
} PreprocEventList;

/* Global event list -- NULL when not recording (compiler mode) */
extern PreprocEventList *g_preproc_events;

void preproc_event_init(PreprocEventList *list);
void preproc_event_cleanup(PreprocEventList *list);

void preproc_event_add_copy(PreprocEventList *list,
                            const char *filename, int line, int col, int end_col,
                            const char *copy_name, const char *resolved_path);

void preproc_event_add_define_ref(PreprocEventList *list,
                                  const char *filename, int line, int col, int end_col,
                                  const char *define_name, const char *define_value,
                                  const char *def_filename, int def_line, int def_col);

PreprocEvent *preproc_event_find_at(PreprocEvent *events, int count,
                                    const char *filename, int line, int col);

#endif /* TBOLC_PREPROC_EVENT_H */
