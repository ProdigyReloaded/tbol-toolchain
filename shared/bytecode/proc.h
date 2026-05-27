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
#ifndef TBOLDC_PROC_H
#define TBOLDC_PROC_H

#include "ir.h"

/*
 * Procedure boundary detection
 */
typedef struct {
    uint16_t start_addr;    /* Procedure entry point */
    uint16_t end_addr;      /* Address after last instruction (RETURN) */
    bool is_main;           /* True for main procedure (addr 0) */
    int proc_num;           /* Procedure number (0 = main, 1+ = called procs) */
} ProcBoundary;

typedef struct {
    ProcBoundary *procs;
    int count;
    int capacity;
} ProcList;

ProcList *proc_list_new(void);
void proc_list_free(ProcList *pl);
void find_procedures(Program *prog, ProcList *pl);

#endif /* TBOLDC_PROC_H */
