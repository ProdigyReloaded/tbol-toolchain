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
 * TBOL Compiler - Symbol Table Implementation
 */

#include "symtab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Hash table sizes */
#define VAR_HASH_SIZE 256
#define PROC_HASH_SIZE 64

/* Hash tables */
static Symbol *var_table[VAR_HASH_SIZE];
static Symbol *proc_table[PROC_HASH_SIZE];

/* Structure group list (for SAVE/CLEAR with structure names) */
static StructureGroup *structure_list = NULL;

/* Data segment tracking */
static int data_offset = 0;
static int proc_order_counter = 0;

/* String size in TBOL (default for unspecified) */
#define DEFAULT_STRING_SIZE 256

/* Hash function (case-insensitive) */
static unsigned int hash_name(const char *name, int table_size) {
    unsigned int hash = 0;
    while (*name) {
        char c = *name++;
        if (c >= 'a' && c <= 'z') c -= 32;
        hash = hash * 31 + (unsigned char)c;
    }
    return hash % table_size;
}

void symtab_init(void) {
    memset(var_table, 0, sizeof(var_table));
    memset(proc_table, 0, sizeof(proc_table));
    structure_list = NULL;
    data_offset = 0;
    proc_order_counter = 0;
}

static void free_labels(Symbol *labels) {
    while (labels) {
        Symbol *next = labels->next;
        free(labels->name);
        free(labels);
        labels = next;
    }
}

void symtab_cleanup(void) {
    /* Free variables */
    for (int i = 0; i < VAR_HASH_SIZE; i++) {
        Symbol *sym = var_table[i];
        while (sym) {
            Symbol *next = sym->next;
            free(sym->name);
            free(sym);
            sym = next;
        }
        var_table[i] = NULL;
    }

    /* Free procedures (and their labels) */
    for (int i = 0; i < PROC_HASH_SIZE; i++) {
        Symbol *sym = proc_table[i];
        while (sym) {
            Symbol *next = sym->next;
            free_labels(sym->data.proc.labels);
            free(sym->name);
            free(sym);
            sym = next;
        }
        proc_table[i] = NULL;
    }

    /* Free structure groups */
    StructureGroup *sg = structure_list;
    while (sg) {
        StructureGroup *next = sg->next;
        free(sg->name);
        free(sg);
        sg = next;
    }
    structure_list = NULL;

    data_offset = 0;
}

Symbol *symtab_define_var(const char *name, int array_size, SourceLoc loc) {
    unsigned int h = hash_name(name, VAR_HASH_SIZE);

    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = SYM_VARIABLE;
    sym->defined_at = loc;
    sym->data.var.array_size = array_size;
    sym->data.var.offset = data_offset;

    /* Calculate size: each element is DEFAULT_STRING_SIZE bytes */
    int count = array_size > 0 ? array_size : 1;
    data_offset += count * DEFAULT_STRING_SIZE;

    /* Add to hash chain */
    sym->next = var_table[h];
    var_table[h] = sym;

    return sym;
}

Symbol *symtab_define_proc(const char *name, AstNode *node, SourceLoc loc) {
    unsigned int h = hash_name(name, PROC_HASH_SIZE);

    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = SYM_PROCEDURE;
    sym->defined_at = loc;
    sym->data.proc.node = node;
    sym->data.proc.labels = NULL;
    sym->data.proc.order = proc_order_counter++;

    sym->next = proc_table[h];
    proc_table[h] = sym;

    return sym;
}

Symbol *symtab_define_label(const char *name, Symbol *proc, SourceLoc loc) {
    if (!proc || proc->kind != SYM_PROCEDURE) {
        return NULL;
    }

    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->kind = SYM_LABEL;
    sym->defined_at = loc;
    sym->data.label.proc = proc;

    /* Add to procedure's label list */
    sym->next = proc->data.proc.labels;
    proc->data.proc.labels = sym;

    return sym;
}

Symbol *symtab_lookup_var(const char *name) {
    unsigned int h = hash_name(name, VAR_HASH_SIZE);
    Symbol *sym = var_table[h];
    while (sym) {
        if (strcasecmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

Symbol *symtab_lookup_proc(const char *name) {
    unsigned int h = hash_name(name, PROC_HASH_SIZE);
    Symbol *sym = proc_table[h];
    while (sym) {
        if (strcasecmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

Symbol *symtab_lookup_label(Symbol *proc, const char *name) {
    if (!proc || proc->kind != SYM_PROCEDURE) {
        return NULL;
    }
    Symbol *sym = proc->data.proc.labels;
    while (sym) {
        if (strcasecmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

bool symtab_var_exists(const char *name) {
    return symtab_lookup_var(name) != NULL;
}

bool symtab_proc_exists(const char *name) {
    return symtab_lookup_proc(name) != NULL;
}

bool symtab_label_exists(Symbol *proc, const char *name) {
    return symtab_lookup_label(proc, name) != NULL;
}

void symtab_define_structure(const char *name, int start_slot, int count) {
    StructureGroup *sg = malloc(sizeof(StructureGroup));
    sg->name = strdup(name);
    sg->start_slot = start_slot;
    sg->count = count;
    sg->next = structure_list;
    structure_list = sg;
}

StructureGroup *symtab_lookup_structure(const char *name) {
    StructureGroup *sg = structure_list;
    while (sg) {
        if (strcasecmp(sg->name, name) == 0) {
            return sg;
        }
        sg = sg->next;
    }
    return NULL;
}

void symtab_dump(void) {
    printf("=== Symbol Table ===\n\n");

    printf("Variables:\n");
    for (int i = 0; i < VAR_HASH_SIZE; i++) {
        Symbol *sym = var_table[i];
        while (sym) {
            if (sym->data.var.array_size > 0) {
                printf("  %s(%d) at offset %d\n",
                       sym->name, sym->data.var.array_size, sym->data.var.offset);
            } else {
                printf("  %s at offset %d\n", sym->name, sym->data.var.offset);
            }
            sym = sym->next;
        }
    }

    printf("\nProcedures:\n");
    for (int i = 0; i < PROC_HASH_SIZE; i++) {
        Symbol *sym = proc_table[i];
        while (sym) {
            printf("  %s\n", sym->name);
            Symbol *label = sym->data.proc.labels;
            while (label) {
                printf("    label: %s\n", label->name);
                label = label->next;
            }
            sym = sym->next;
        }
    }

    printf("\nTotal data size: %d bytes\n", data_offset);
}
