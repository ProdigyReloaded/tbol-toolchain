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
 * TBOL Compiler - Preprocessor (COPY/DEFINE handling)
 */

#ifndef TBOLC_PREPROC_H
#define TBOLC_PREPROC_H

#include <stdbool.h>

/* Maximum nesting depth for COPY files */
#define MAX_INCLUDE_DEPTH 32

/* Initialize/cleanup preprocessor state */
void preproc_init(void);
void preproc_cleanup(void);

/* Include path management */
void preproc_add_include_path(const char *path);
void preproc_set_current_dir(const char *dir);

/* Canonicalize a path to absolute form: realpath() on POSIX, _fullpath() on
 * Windows (with backslashes normalized to forward slashes). Never returns NULL
 * -- falls back to a copy of the input on failure. Caller frees. */
char *preproc_canonicalize_path(const char *path);

/*
 * COPY file handling
 *
 * preproc_push_file() searches for the file and pushes it onto the include stack.
 * Returns 0 on success, -1 on error (file not found, cycle detected, etc.)
 * The error message is set via diag_error().
 */
int preproc_push_file(const char *filename);

/*
 * Called by lexer at EOF to pop the include stack.
 * Returns true if there are more files to process, false if done.
 */
bool preproc_pop_file(void);

/* Get current include depth (0 = main file) */
int preproc_get_include_depth(void);

/* Get the current file being processed */
const char *preproc_get_current_file(void);

/* Get the last resolved COPY file path (for event recording) */
const char *preproc_get_last_resolved_path(void);

/*
 * DEFINE handling
 *
 * preproc_add_define() registers a substitution.
 * preproc_lookup_define() returns the substitution value or NULL.
 *
 * add_define takes line/col for warning on redefinition.
 */
void preproc_add_define(const char *name, const char *value, int line, int col);
const char *preproc_lookup_define(const char *name);

/* Iterate every registered DEFINE (name + expansion value). Used to export
 * GEV/PEV symbol names (DEFINEs whose value is #N / &N) to .sdb debug info. */
void preproc_foreach_define(void (*fn)(const char *name, const char *value, void *ctx),
                            void *ctx);

/* Check if a name is defined */
bool preproc_is_defined(const char *name);

/* DEFINE info for LSP (go-to-definition on DEFINE references) */
typedef struct {
    const char *value;      /* Expansion value (borrowed pointer) */
    const char *filename;   /* File where DEFINE was declared (borrowed) */
    int line, column;
} PreprocDefineInfo;

bool preproc_lookup_define_info(const char *name, PreprocDefineInfo *info);

/*
 * DEFINE expansion cycle detection
 *
 * Track which DEFINEs are currently being expanded to detect cycles.
 */
void preproc_clear_expansions(void);
void preproc_push_expansion(const char *name);
bool preproc_is_expanding(const char *name);

/*
 * Content overrides for COPY files
 *
 * When set, preproc_push_file() will use the override content instead of
 * reading from disk. Used by the LSP to inject dirty editor buffers.
 */
void preproc_set_content_override(const char *filepath, const char *content);
void preproc_clear_content_overrides(void);

/*
 * Lexer filename pool cleanup
 *
 * The lexer maintains an interned string pool for YYLTYPE filenames.
 * Call this when done parsing to free pooled strings.
 */
void lexer_clear_filename_pool(void);

/*
 * Full lexer cleanup - frees flex buffers and resets all state.
 * Must be called between successive parses to prevent buffer leaks.
 */
void lexer_cleanup(void);

#endif /* TBOLC_PREPROC_H */
