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
 * TBOL Preprocessor Events - Implementation
 */

#include "preproc_event.h"
#include <stdlib.h>
#include <string.h>

/* Global event list pointer -- NULL unless LSP is recording */
PreprocEventList *g_preproc_events = NULL;

void preproc_event_init(PreprocEventList *list) {
    list->events = NULL;
    list->count = 0;
    list->capacity = 0;
}

static void event_free_data(PreprocEvent *evt) {
    free(evt->filename);
    free(evt->name);
    free(evt->resolved_path);
    free(evt->define_value);
    free(evt->def_filename);
}

void preproc_event_cleanup(PreprocEventList *list) {
    for (int i = 0; i < list->count; i++) {
        event_free_data(&list->events[i]);
    }
    free(list->events);
    list->events = NULL;
    list->count = 0;
    list->capacity = 0;
}

static PreprocEvent *event_alloc(PreprocEventList *list) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity ? list->capacity * 2 : 16;
        list->events = realloc(list->events, new_cap * sizeof(PreprocEvent));
        list->capacity = new_cap;
    }
    PreprocEvent *evt = &list->events[list->count++];
    memset(evt, 0, sizeof(*evt));
    return evt;
}

void preproc_event_add_copy(PreprocEventList *list,
                            const char *filename, int line, int col, int end_col,
                            const char *copy_name, const char *resolved_path) {
    if (!list) return;
    PreprocEvent *evt = event_alloc(list);
    evt->kind = PREPROC_EVENT_COPY;
    evt->filename = filename ? strdup(filename) : NULL;
    evt->line = line;
    evt->column = col;
    evt->end_column = end_col;
    evt->name = copy_name ? strdup(copy_name) : NULL;
    evt->resolved_path = resolved_path ? strdup(resolved_path) : NULL;
}

void preproc_event_add_define_ref(PreprocEventList *list,
                                  const char *filename, int line, int col, int end_col,
                                  const char *define_name, const char *define_value,
                                  const char *def_filename, int def_line, int def_col) {
    if (!list) return;
    PreprocEvent *evt = event_alloc(list);
    evt->kind = PREPROC_EVENT_DEFINE_REF;
    evt->filename = filename ? strdup(filename) : NULL;
    evt->line = line;
    evt->column = col;
    evt->end_column = end_col;
    evt->name = define_name ? strdup(define_name) : NULL;
    evt->define_value = define_value ? strdup(define_value) : NULL;
    evt->def_filename = def_filename ? strdup(def_filename) : NULL;
    evt->def_line = def_line;
    evt->def_column = def_col;
}

PreprocEvent *preproc_event_find_at(PreprocEvent *events, int count,
                                    const char *filename, int line, int col) {
    for (int i = 0; i < count; i++) {
        PreprocEvent *evt = &events[i];
        if (evt->line == line && col >= evt->column && col <= evt->end_column) {
            /* Filter by filename — events from COPY files have transparent
             * line numbers that would falsely match the main file. */
            if (filename && evt->filename && strcmp(evt->filename, filename) != 0)
                continue;
            return evt;
        }
    }
    return NULL;
}
