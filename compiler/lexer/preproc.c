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
 * TBOL Compiler - Preprocessor Implementation
 */

#include "preproc.h"
#include "../options.h"
#include "../diag/diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>  /* PATH_MAX for realpath */

/* Include stack entry */
typedef struct {
    char *filename;         /* Full path to file */
    int saved_line;         /* Line number when we left this file */
    int saved_column;       /* Column when we left this file */
} IncludeEntry;

/* Include stack */
static IncludeEntry include_stack[MAX_INCLUDE_DEPTH];
static int include_depth = 0;

/* Current directory (for relative includes) */
static char *current_dir = NULL;

/* Last resolved COPY file path (for event recording) */
static char *last_resolved_copy_path = NULL;

/* DEFINE table entry */
typedef struct DefineEntry {
    char *name;
    char *value;
    char *def_filename;     /* File where DEFINE was declared */
    int def_line;
    int def_column;
    struct DefineEntry *next;
} DefineEntry;

/* Simple hash table for DEFINEs */
#define DEFINE_HASH_SIZE 64
static DefineEntry *define_table[DEFINE_HASH_SIZE];

/* DEFINE expansion tracking for cycle detection */
#define MAX_EXPANSION_DEPTH 32
static const char *expansion_stack[MAX_EXPANSION_DEPTH];
static int expansion_depth = 0;

/* Content overrides for COPY files (LSP dirty buffer injection) */
#define MAX_CONTENT_OVERRIDES 16
typedef struct {
    char *path;
    char *content;
} ContentOverride;
static ContentOverride content_overrides[MAX_CONTENT_OVERRIDES];
static int content_override_count = 0;

/* External declarations for flex integration - these are in the lexer */
extern FILE *yyin;
extern void lexer_set_filename(const char *filename);
extern void lexer_set_filename_only(const char *filename);
extern void lexer_push_buffer(FILE *fp);
extern void lexer_pop_buffer(void);

/* Hash function for DEFINE names */
static unsigned int hash_name(const char *name) {
    unsigned int hash = 0;
    while (*name) {
        hash = hash * 31 + (unsigned char)*name++;
    }
    return hash % DEFINE_HASH_SIZE;
}

void preproc_init(void) {
    include_depth = 0;
    current_dir = NULL;
    memset(define_table, 0, sizeof(define_table));
}

void preproc_cleanup(void) {
    /* Clean up include stack */
    while (include_depth > 0) {
        include_depth--;
        free(include_stack[include_depth].filename);
        include_stack[include_depth].filename = NULL;
    }

    /* Clean up current directory */
    free(current_dir);
    current_dir = NULL;

    /* Clean up last resolved copy path */
    free(last_resolved_copy_path);
    last_resolved_copy_path = NULL;

    /* Clean up DEFINE table */
    for (int i = 0; i < DEFINE_HASH_SIZE; i++) {
        DefineEntry *entry = define_table[i];
        while (entry) {
            DefineEntry *next = entry->next;
            free(entry->name);
            free(entry->value);
            free(entry->def_filename);
            free(entry);
            entry = next;
        }
        define_table[i] = NULL;
    }
}

void preproc_set_current_dir(const char *dir) {
    free(current_dir);
    current_dir = dir ? strdup(dir) : NULL;
}

void preproc_add_include_path(const char *path) {
    /* Include paths are managed in g_options */
    if (g_options.include_path_count < MAX_INCLUDE_PATHS) {
        g_options.include_paths[g_options.include_path_count++] = path;
    }
}

/* Canonicalize a path with realpath() (POSIX) or _fullpath() (Windows),
 * falling back to the original on failure.  On Windows, also normalize
 * backslashes to forward slashes so paths compare equal against URI-derived
 * paths everywhere in the toolchain. */
char *preproc_canonicalize_path(const char *path) {
    char resolved[PATH_MAX];
#ifdef _WIN32
    if (_fullpath(resolved, path, sizeof(resolved))) {
        for (char *p = resolved; *p; p++) if (*p == '\\') *p = '/';
        return strdup(resolved);
    }
#else
    if (realpath(path, resolved)) {
        return strdup(resolved);
    }
#endif
    return strdup(path);
}

/* Search for a COPY file in include paths */
static char *find_copy_file(const char *name) {
    char path[4096];
    FILE *fp;

    /* COPY files have no extension - use the name directly */

    /* Try current directory first */
    if (current_dir) {
        snprintf(path, sizeof(path), "%s/%s", current_dir, name);
        fp = fopen(path, "r");
        if (fp) {
            fclose(fp);
            return preproc_canonicalize_path(path);
        }
    }

    /* Try current working directory */
    fp = fopen(name, "r");
    if (fp) {
        fclose(fp);
        return preproc_canonicalize_path(name);
    }

    /* Try -I paths */
    for (int i = 0; i < g_options.include_path_count; i++) {
        snprintf(path, sizeof(path), "%s/%s", g_options.include_paths[i], name);
        fp = fopen(path, "r");
        if (fp) {
            fclose(fp);
            return preproc_canonicalize_path(path);
        }
    }

    return NULL;
}

/* Check if a file is already in the include stack (cycle detection) */
static bool is_in_include_stack(const char *filename) {
    for (int i = 0; i < include_depth; i++) {
        if (strcmp(include_stack[i].filename, filename) == 0) {
            return true;
        }
    }
    return false;
}

int preproc_push_file(const char *name) {
    extern int current_line;
    extern int current_column;

    /* Check depth limit */
    if (include_depth >= MAX_INCLUDE_DEPTH) {
        SourceLoc loc = {preproc_get_current_file(), current_line, current_column};
        diag_error(loc, "maximum include depth (%d) exceeded", MAX_INCLUDE_DEPTH);
        return -1;
    }

    /* Find the file */
    char *filepath = find_copy_file(name);
    if (!filepath) {
        SourceLoc loc = {preproc_get_current_file(), current_line, current_column};
        diag_error(loc, "COPY file '%s' not found", name);
        return -1;
    }

    /* Check for cycles */
    if (is_in_include_stack(filepath)) {
        SourceLoc loc = {preproc_get_current_file(), current_line, current_column};
        diag_error(loc, "circular COPY detected: '%s'", filepath);
        free(filepath);
        return -1;
    }

    /* Check for content override (dirty editor buffer from LSP) */
    FILE *fp = NULL;
    for (int i = 0; i < content_override_count; i++) {
        if (strcmp(content_overrides[i].path, filepath) == 0) {
            fp = tmpfile();
            if (fp) {
                const char *ovr = content_overrides[i].content;
                fwrite(ovr, 1, strlen(ovr), fp);
                rewind(fp);
            }
            break;
        }
    }

    /* Fall back to reading from disk */
    if (!fp) {
        fp = fopen(filepath, "r");
    }
    if (!fp) {
        SourceLoc loc = {preproc_get_current_file(), current_line, current_column};
        diag_error(loc, "cannot open COPY file '%s'", filepath);
        free(filepath);
        return -1;
    }

    /* Save current state */
    include_stack[include_depth].filename = strdup(preproc_get_current_file() ? preproc_get_current_file() : "<unknown>");
    include_stack[include_depth].saved_line = current_line;
    include_stack[include_depth].saved_column = current_column;
    include_depth++;

    /* Switch to new file - let the lexer handle buffer management */
    /* Use lexer_set_filename_only to preserve line numbers (COPY transparency) */
    lexer_push_buffer(fp);
    lexer_set_filename_only(filepath);

    if (g_options.verbose) {
        fprintf(stderr, "Including %s\n", filepath);
    }

    /* Save resolved path for event recording */
    free(last_resolved_copy_path);
    last_resolved_copy_path = filepath;  /* Transfer ownership */
    return 0;
}

bool preproc_pop_file(void) {
    extern int current_line;
    extern int current_column;

    if (include_depth == 0) {
        return false;  /* No more files */
    }

    /* Pop the buffer - let the lexer handle cleanup */
    lexer_pop_buffer();

    /* Pop the stack */
    include_depth--;
    IncludeEntry *entry = &include_stack[include_depth];

    /* Restore previous state */
    lexer_set_filename(entry->filename);
    current_line = entry->saved_line;
    current_column = entry->saved_column;

    free(entry->filename);  /* Lexer made its own copy */
    entry->filename = NULL;

    return true;
}

int preproc_get_include_depth(void) {
    return include_depth;
}

const char *preproc_get_current_file(void) {
    extern const char *lexer_get_filename(void);
    return lexer_get_filename();
}

void preproc_foreach_define(void (*fn)(const char *name, const char *value, void *ctx),
                            void *ctx) {
    for (int i = 0; i < DEFINE_HASH_SIZE; i++)
        for (DefineEntry *e = define_table[i]; e; e = e->next)
            fn(e->name, e->value, ctx);
}

void preproc_add_define(const char *name, const char *value, int line, int col) {
    unsigned int h = hash_name(name);
    const char *cur_file = preproc_get_current_file();

    /*
     * Compute actual line within the current file.
     * COPY transparency means current_line is never reset, so inside a COPY
     * file `line` is offset by the parent's line.  Subtract the saved_line
     * from the include stack to recover the real line in the current file.
     */
    int actual_line = line;
    if (include_depth > 0) {
        actual_line = line - include_stack[include_depth - 1].saved_line + 1;
        if (actual_line < 1) actual_line = 1;  /* safety */
    }

    /* Check if already defined */
    DefineEntry *entry = define_table[h];
    while (entry) {
        if (strcasecmp(entry->name, name) == 0) {
            /* Redefine - warn if enabled (diagnostic uses transparent line) */
            if (g_options.warn_redefine) {
                SourceLoc loc = {cur_file, line, col};
                diag_warning(loc, "redefining '%s'", name);
            }
            free(entry->value);
            entry->value = strdup(value);
            free(entry->def_filename);
            entry->def_filename = cur_file ? strdup(cur_file) : NULL;
            entry->def_line = actual_line;
            entry->def_column = col;
            return;
        }
        entry = entry->next;
    }

    /* Add new entry */
    entry = malloc(sizeof(DefineEntry));
    entry->name = strdup(name);
    entry->value = strdup(value);
    entry->def_filename = cur_file ? strdup(cur_file) : NULL;
    entry->def_line = actual_line;
    entry->def_column = col;
    entry->next = define_table[h];
    define_table[h] = entry;
}

const char *preproc_lookup_define(const char *name) {
    unsigned int h = hash_name(name);
    DefineEntry *entry = define_table[h];
    while (entry) {
        if (strcasecmp(entry->name, name) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    return NULL;
}

const char *preproc_get_last_resolved_path(void) {
    return last_resolved_copy_path;
}

bool preproc_is_defined(const char *name) {
    return preproc_lookup_define(name) != NULL;
}

bool preproc_lookup_define_info(const char *name, PreprocDefineInfo *info) {
    unsigned int h = hash_name(name);
    DefineEntry *entry = define_table[h];
    while (entry) {
        if (strcasecmp(entry->name, name) == 0) {
            info->value = entry->value;
            info->filename = entry->def_filename;
            info->line = entry->def_line;
            info->column = entry->def_column;
            return true;
        }
        entry = entry->next;
    }
    return false;
}

/*
 * DEFINE expansion cycle detection
 */

void preproc_clear_expansions(void) {
    for (int i = 0; i < expansion_depth; i++) {
        free((void *)expansion_stack[i]);
        expansion_stack[i] = NULL;
    }
    expansion_depth = 0;
}

void preproc_push_expansion(const char *name) {
    if (expansion_depth < MAX_EXPANSION_DEPTH) {
        expansion_stack[expansion_depth++] = strdup(name);
    }
}

bool preproc_is_expanding(const char *name) {
    for (int i = 0; i < expansion_depth; i++) {
        if (strcasecmp(expansion_stack[i], name) == 0) {
            return true;
        }
    }
    return false;
}

/*
 * Content override management (for LSP dirty buffer injection)
 */
void preproc_set_content_override(const char *filepath, const char *content) {
    if (!filepath || !content) return;

    /* Canonicalize path to match find_copy_file() output */
    char *canon = preproc_canonicalize_path(filepath);

    /* Check if already exists - update it */
    for (int i = 0; i < content_override_count; i++) {
        if (strcmp(content_overrides[i].path, canon) == 0) {
            free(content_overrides[i].content);
            content_overrides[i].content = strdup(content);
            free(canon);
            return;
        }
    }

    /* Add new override */
    if (content_override_count < MAX_CONTENT_OVERRIDES) {
        content_overrides[content_override_count].path = canon;
        content_overrides[content_override_count].content = strdup(content);
        content_override_count++;
    } else {
        free(canon);
    }
}

void preproc_clear_content_overrides(void) {
    for (int i = 0; i < content_override_count; i++) {
        free(content_overrides[i].path);
        free(content_overrides[i].content);
        content_overrides[i].path = NULL;
        content_overrides[i].content = NULL;
    }
    content_override_count = 0;
}
