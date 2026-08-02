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
#include "proc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Procedure boundary detection
 */
ProcList *proc_list_new(void) {
    ProcList *pl = calloc(1, sizeof(ProcList));
    pl->capacity = 16;
    pl->procs = calloc(pl->capacity, sizeof(ProcBoundary));
    return pl;
}

void proc_list_free(ProcList *pl) {
    if (!pl) return;
    free(pl->procs);
    free(pl);
}

static void proc_list_add(ProcList *pl, uint16_t start, uint16_t end, bool is_main, int num) {
    if (pl->count >= pl->capacity) {
        pl->capacity *= 2;
        ProcBoundary *tmp = realloc(pl->procs, pl->capacity * sizeof(ProcBoundary));
        if (!tmp) { fprintf(stderr, "out of memory\n"); exit(1); }
        pl->procs = tmp;
    }
    pl->procs[pl->count].start_addr = start;
    pl->procs[pl->count].end_addr = end;
    pl->procs[pl->count].is_main = is_main;
    pl->procs[pl->count].proc_num = num;
    pl->count++;
}

/* Compare function for sorting call targets */
static int cmp_uint16(const void *a, const void *b) {
    uint16_t ua = *(const uint16_t *)a;
    uint16_t ub = *(const uint16_t *)b;
    return (ua > ub) - (ua < ub);
}

/*
 * Find all procedure boundaries in a program
 * Procedures are delimited by CALL targets and RETURN instructions
 */
void find_procedures(Program *prog, ProcList *pl) {
    if (!prog || !pl) return;

    /* Collect all CALL targets (procedure entry points) */
    uint16_t *entries = calloc(256, sizeof(uint16_t));
    int entry_count = 0;

    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        if (instr->mnemonic == MNEM_CALL && instr->has_jump) {
            /* Check if this target is already recorded */
            bool found = false;
            for (int i = 0; i < entry_count; i++) {
                if (entries[i] == instr->jump_target) {
                    found = true;
                    break;
                }
            }
            /* Jump offset 0 means unresolved (call to undefined proc) - skip it */
            if (!found && entry_count < 256 && instr->jump_offset != 0) {
                entries[entry_count++] = instr->jump_target;
            }
        }
    }

    /* Sort entry points by address */
    qsort(entries, entry_count, sizeof(uint16_t), cmp_uint16);

    /* Remove CALL entries that are also GOTO_DEPENDING_ON targets in main.
     * GDO targets define case labels within main's dispatch table - they are
     * inline branch targets, not procedure boundaries. Only filter entries
     * that are *exactly* GDO operand targets; CALL targets that happen to
     * fall within the dispatch address range but are NOT GDO targets are
     * real procedures (e.g., a proc called from within a GDO case handler). */
    {
        uint16_t gdo_targets[256];
        int gdo_target_count = 0;
        for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
            if (entry_count > 0 && instr->address >= entries[0]) break;
            if (instr->mnemonic == MNEM_GOTO_DEPENDING_ON) {
                for (int i = 1; i < instr->operand_count; i++) {
                    if (gdo_target_count < 256)
                        gdo_targets[gdo_target_count++] = (uint16_t)instr->operands[i].value;
                }
            }
        }
        if (gdo_target_count > 0) {
            int new_count = 0;
            for (int i = 0; i < entry_count; i++) {
                bool is_gdo_target = false;
                for (int g = 0; g < gdo_target_count; g++) {
                    if (entries[i] == gdo_targets[g]) {
                        is_gdo_target = true;
                        break;
                    }
                }
                if (!is_gdo_target)
                    entries[new_count++] = entries[i];
            }
            entry_count = new_count;
        }
    }

    /* Find procedure boundaries */
    /* Main procedure: from start to the last RETURN before any called proc entry
     * (using last RETURN handles GOTO_DEPENDING_ON with embedded RETURNs) */
    uint16_t main_end = 0;
    uint16_t first_called_proc = (entry_count > 0) ? entries[0] : UINT16_MAX;
    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        /* Stop at first called procedure entry */
        if (instr->address >= first_called_proc) break;
        if (instr->mnemonic == MNEM_RETURN) {
            /* Use the last RETURN, not the first */
            main_end = instr->address + instr->length;
        }
        /* Track end of last instruction as fallback for programs without RETURN */
        if (!instr->next && main_end == 0) {
            main_end = instr->address + instr->length;
        }
    }
    if (main_end > 0) {
        proc_list_add(pl, 0, main_end, true, 0);
    }

    /* Find boundaries for each called procedure */
    int proc_num = 1;
    for (int i = 0; i < entry_count; i++) {
        uint16_t start = entries[i];
        uint16_t end = 0;

        /* Find the LAST RETURN that ends this procedure (before next proc entry).
         * Procedures may have early RETURNs inside IF bodies. */
        for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
            if (instr->address < start) continue;
            /* Stop if we hit another procedure's entry point */
            if (i + 1 < entry_count && instr->address >= entries[i + 1]) {
                break;
            }
            if (instr->mnemonic == MNEM_RETURN) {
                end = instr->address + instr->length;
                /* Don't break - keep looking for later RETURNs */
            }
        }

        if (end > start) {
            proc_list_add(pl, start, end, false, proc_num++);
        }
    }

    free(entries);
}
