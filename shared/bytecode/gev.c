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
#include "gev.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * GEV (Global Environment Variable) name resolution
 */

GEVTable *gev_table_new(void) {
    GEVTable *gt = calloc(1, sizeof(GEVTable));
    gt->capacity = 256;
    gt->entries = calloc(gt->capacity, sizeof(GEVEntry));
    return gt;
}

void gev_table_free(GEVTable *gt) {
    if (!gt) return;
    for (int i = 0; i < gt->count; i++) {
        free(gt->entries[i].name);
    }
    free(gt->entries);
    free(gt);
}

static void gev_table_add(GEVTable *gt, int number, const char *name) {
    if (gt->count >= gt->capacity) {
        gt->capacity *= 2;
        GEVEntry *tmp = realloc(gt->entries, gt->capacity * sizeof(GEVEntry));
        if (!tmp) { fprintf(stderr, "out of memory\n"); exit(1); }
        gt->entries = tmp;
    }
    gt->entries[gt->count].number = number;
    gt->entries[gt->count].name = strdup(name);
    gt->count++;
}

/*
 * Parse a single DEFINE line from XXCGTSYS
 * Format: DEFINE <name> ,#<number>;
 * Returns true if successfully parsed
 */
static bool parse_define_line(const char *line, char *name_out, int *number_out) {
    /* Skip leading whitespace */
    while (*line == ' ' || *line == '\t') line++;

    /* Check for DEFINE keyword */
    if (strncmp(line, "DEFINE", 6) != 0) return false;
    line += 6;

    /* Skip whitespace after DEFINE */
    while (*line == ' ' || *line == '\t') line++;

    /* Extract name (until comma or whitespace) */
    char *np = name_out;
    while (*line && *line != ',' && *line != ' ' && *line != '\t' && *line != ';') {
        *np++ = *line++;
    }
    *np = '\0';

    if (name_out[0] == '\0') return false;

    /* Find the ,# pattern */
    const char *hash = strstr(line, ",#");
    if (!hash) return false;
    hash += 2;  /* Skip ,# */

    /* Parse number */
    int num = 0;
    while (*hash >= '0' && *hash <= '9') {
        num = num * 10 + (*hash - '0');
        hash++;
    }

    if (num == 0) return false;  /* No valid number found */

    *number_out = num;
    return true;
}

/*
 * Load GEV definitions from XXCGTSYS file
 * Searches in include paths for the file
 */
bool gev_table_load(GEVTable *gt, const char **include_paths, int path_count) {
    if (!gt) return false;

    FILE *f = NULL;
    char filepath[1024];

    /* Try current directory first */
    f = fopen("XXCGTSYS", "r");

    /* Try each include path */
    if (!f) {
        for (int i = 0; i < path_count && !f; i++) {
            snprintf(filepath, sizeof(filepath), "%s/XXCGTSYS", include_paths[i]);
            f = fopen(filepath, "r");
        }
    }

    if (!f) {
        gt->loaded = false;
        return false;
    }

    char line[512];
    char name[128];
    int number;

    while (fgets(line, sizeof(line), f)) {
        if (parse_define_line(line, name, &number)) {
            gev_table_add(gt, number, name);
        }
    }

    fclose(f);
    gt->loaded = true;
    return true;
}

/*
 * Look up a GEV number and return its symbolic name
 * Returns NULL if not found
 */
const char *gev_lookup(GEVTable *gt, int number) {
    if (!gt || !gt->loaded) return NULL;

    for (int i = 0; i < gt->count; i++) {
        if (gt->entries[i].number == number) {
            gt->used = true;  /* Mark that we resolved at least one GEV */
            return gt->entries[i].name;
        }
    }
    return NULL;
}
