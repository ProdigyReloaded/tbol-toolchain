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
#ifndef TBOLDC_GEV_H
#define TBOLDC_GEV_H

#include <stdint.h>
#include <stdbool.h>

/*
 * GEV (Global Environment Variable) name resolution
 */
typedef struct {
    int number;         /* GEV number (e.g., 1 for #1) */
    char *name;         /* Symbolic name (e.g., "SYS_RETURN_CODE") */
} GEVEntry;

typedef struct {
    GEVEntry *entries;
    int count;
    int capacity;
    bool loaded;        /* True if XXCGTSYS was found and loaded */
    bool used;          /* True if any GEV was resolved during emit */
} GEVTable;

GEVTable *gev_table_new(void);
void gev_table_free(GEVTable *gt);
bool gev_table_load(GEVTable *gt, const char **include_paths, int path_count);
void gev_table_load_builtin(GEVTable *gt);
bool gev_write_xxcgtsys(const char *dir);
const char *gev_lookup(GEVTable *gt, int number);

#endif /* TBOLDC_GEV_H */
