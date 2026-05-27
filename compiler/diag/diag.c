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
 * TBOL Compiler - Diagnostic Infrastructure Implementation
 */

#include "diag.h"
#include "../options.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define INITIAL_DIAG_CAPACITY 16
#define MAX_SOURCE_LINES 1024

/* Global diagnostic context */
DiagContext g_diag = {0};

/* Source line cache for error context */
typedef struct {
    char *filename;
    int line;
    char *text;
} SourceLineCache;

static SourceLineCache source_cache[MAX_SOURCE_LINES];
static int source_cache_count = 0;

void diag_init(void) {
    memset(&g_diag, 0, sizeof(g_diag));
}

void diag_cleanup(void) {
    for (int i = 0; i < g_diag.count; i++) {
        free((void *)g_diag.diagnostics[i].loc.filename);
        free(g_diag.diagnostics[i].message);
        free(g_diag.diagnostics[i].source_line);
    }
    free(g_diag.diagnostics);

    for (int i = 0; i < source_cache_count; i++) {
        free(source_cache[i].filename);
        free(source_cache[i].text);
    }

    memset(&g_diag, 0, sizeof(g_diag));
    source_cache_count = 0;
}

void diag_set_source_line(const char *filename, int line, const char *text) {
    if (source_cache_count >= MAX_SOURCE_LINES) {
        /* Evict oldest entry */
        free(source_cache[0].filename);
        free(source_cache[0].text);
        memmove(&source_cache[0], &source_cache[1],
                (MAX_SOURCE_LINES - 1) * sizeof(SourceLineCache));
        source_cache_count--;
    }

    SourceLineCache *entry = &source_cache[source_cache_count++];
    entry->filename = strdup(filename ? filename : "<unknown>");
    entry->line = line;
    entry->text = strdup(text ? text : "");
}

static const char *get_source_line(const char *filename, int line) {
    /* Search cache from most recent */
    for (int i = source_cache_count - 1; i >= 0; i--) {
        if (source_cache[i].line == line &&
            strcmp(source_cache[i].filename, filename ? filename : "<unknown>") == 0) {
            return source_cache[i].text;
        }
    }
    return NULL;
}

static void add_diagnostic(DiagLevel level, SourceLoc loc,
                           int highlight_len, const char *fmt, va_list args) {
    /* Expand capacity if needed */
    if (g_diag.count >= g_diag.capacity) {
        int new_cap = g_diag.capacity == 0
            ? INITIAL_DIAG_CAPACITY
            : g_diag.capacity * 2;
        Diagnostic *new_diags = realloc(g_diag.diagnostics,
                                        new_cap * sizeof(Diagnostic));
        if (!new_diags) return;
        g_diag.diagnostics = new_diags;
        g_diag.capacity = new_cap;
    }

    Diagnostic *diag = &g_diag.diagnostics[g_diag.count++];
    diag->level = level;
    diag->loc = loc;
    /* Strdup filename since it may be freed when switching files */
    if (loc.filename) {
        diag->loc.filename = strdup(loc.filename);
    }

    /* Format message */
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    diag->message = strdup(buf);

    /* Get source line for context */
    const char *src = get_source_line(loc.filename, loc.line);
    diag->source_line = src ? strdup(src) : NULL;
    diag->highlight_start = loc.column > 0 ? loc.column - 1 : 0;
    diag->highlight_len = highlight_len > 0 ? highlight_len : 1;

    /* Update counts */
    if (level == DIAG_ERROR) {
        g_diag.error_count++;
    } else if (level == DIAG_WARNING) {
        g_diag.warning_count++;
        if (g_options.warnings_as_errors) {
            g_diag.error_count++;
        }
    }
}

void diag_error(SourceLoc loc, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    add_diagnostic(DIAG_ERROR, loc, 1, fmt, args);
    va_end(args);
}

void diag_warning(SourceLoc loc, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    add_diagnostic(DIAG_WARNING, loc, 1, fmt, args);
    va_end(args);
}

/* ANSI color codes */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_WHITE   "\033[1;37m"
#define COLOR_GREEN   "\033[1;32m"

static void print_diagnostic_modern(const Diagnostic *diag) {
    const char *level_str;
    const char *color;

    switch (diag->level) {
        case DIAG_ERROR:
            level_str = "error";
            color = COLOR_RED;
            break;
        case DIAG_WARNING:
            level_str = "warning";
            color = COLOR_YELLOW;
            break;
        case DIAG_NOTE:
            level_str = "note";
            color = COLOR_CYAN;
            break;
        default:
            level_str = "unknown";
            color = COLOR_WHITE;
    }

    /* Print location and message */
    fprintf(stderr, "%s%s:%d:%d:%s %s%s:%s %s\n",
            COLOR_WHITE,
            diag->loc.filename ? diag->loc.filename : "<unknown>",
            diag->loc.line,
            diag->loc.column,
            COLOR_RESET,
            color,
            level_str,
            COLOR_RESET,
            diag->message);

    /* Print source line with caret */
    if (diag->source_line) {
        fprintf(stderr, "    %s\n", diag->source_line);

        /* Print caret line */
        fprintf(stderr, "    ");
        for (int i = 0; i < diag->highlight_start; i++) {
            fputc(diag->source_line[i] == '\t' ? '\t' : ' ', stderr);
        }
        fprintf(stderr, "%s^", color);
        for (int i = 1; i < diag->highlight_len; i++) {
            fputc('~', stderr);
        }
        fprintf(stderr, "%s\n", COLOR_RESET);
    }
}

static void print_diagnostic_compat(const Diagnostic *diag) {
    /* Original TBOL 4.21 format: FILENAME.SRC(line)<tab>message */
    fprintf(stderr, "%s(%d)\t%s\n",
            diag->loc.filename ? diag->loc.filename : "<unknown>",
            diag->loc.line,
            diag->message);
}

static void print_diagnostic_json(const Diagnostic *diag) {
    const char *level_str;
    switch (diag->level) {
        case DIAG_ERROR:   level_str = "error"; break;
        case DIAG_WARNING: level_str = "warning"; break;
        case DIAG_NOTE:    level_str = "note"; break;
        default:           level_str = "unknown";
    }

    printf("{\"level\":\"%s\",\"file\":\"%s\",\"line\":%d,\"column\":%d,\"message\":\"%s\"}\n",
           level_str,
           diag->loc.filename ? diag->loc.filename : "",
           diag->loc.line,
           diag->loc.column,
           diag->message);
}

void diag_print_all(void) {
    for (int i = 0; i < g_diag.count; i++) {
        const Diagnostic *diag = &g_diag.diagnostics[i];

        switch (g_options.diag_format) {
            case DIAG_FMT_TEXT:
                print_diagnostic_modern(diag);
                break;
            case DIAG_FMT_COMPAT:
                print_diagnostic_compat(diag);
                break;
            case DIAG_FMT_JSON:
                print_diagnostic_json(diag);
                break;
        }
    }
}

void diag_print_summary(void) {
    if (g_options.diag_format == DIAG_FMT_JSON) {
        return;  /* No summary in JSON mode */
    }

    if (g_diag.error_count == 0 && g_diag.warning_count == 0) {
        return;
    }

    fprintf(stderr, "\n");
    if (g_diag.error_count > 0) {
        fprintf(stderr, "%d error%s",
                g_diag.error_count,
                g_diag.error_count == 1 ? "" : "s");
    }
    if (g_diag.warning_count > 0) {
        if (g_diag.error_count > 0) {
            fprintf(stderr, ", ");
        }
        fprintf(stderr, "%d warning%s",
                g_diag.warning_count,
                g_diag.warning_count == 1 ? "" : "s");
    }
    fprintf(stderr, " generated.\n");
}

bool diag_has_errors(void) {
    return g_diag.error_count > 0;
}

int diag_error_count(void) {
    return g_diag.error_count;
}

int diag_warning_count(void) {
    return g_diag.warning_count;
}
