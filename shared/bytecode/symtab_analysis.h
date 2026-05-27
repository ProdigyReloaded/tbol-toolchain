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
#ifndef TBOLDC_SYMTAB_ANALYSIS_H
#define TBOLDC_SYMTAB_ANALYSIS_H

#include "ir.h"

#define MAX_RDA_SLOTS 222

/*
 * Symbol table for RDA slot tracking
 */
typedef struct {
    bool used;              /* Slot is referenced somewhere */
    bool is_array;          /* Has indexed access */
    bool direct_access;     /* Has non-indexed access */
    int max_index;          /* Highest literal index seen */
    bool bounds_certain;    /* true if only literal indices used */
    int index_ireg;         /* I register used as index (1-8), 0 if varies/unknown */
} RDASlotInfo;

/*
 * Range operation (CLEAR/SAVE on structure)
 */
typedef struct {
    int start_slot;         /* Starting RDA slot */
    int count;              /* Number of consecutive slots */
    bool is_confirmed_struct; /* True if interior slot has indexed access (proves struct, not array) */
    char group_name[16];    /* Generated group name for confirmed structs (e.g., "dt") */
} RangeOp;

typedef struct {
    RDASlotInfo slots[MAX_RDA_SLOTS]; /* RDA0-RDA221 */
    int max_slot;           /* Highest slot number referenced (-1 if none) */
    int ireg_max[9];        /* Max literal value compared to I1-I8 (0 = unknown) */
    RangeOp ranges[32];     /* Observed CLEAR/SAVE range operations */
    int range_count;
} SymbolTable;

/* Symbol table functions */
SymbolTable *symbol_table_new(void);
void symbol_table_free(SymbolTable *st);
void symbol_table_scan(SymbolTable *st, Program *prog);

#endif /* TBOLDC_SYMTAB_ANALYSIS_H */
