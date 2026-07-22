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
 * TBOL Compiler - Symbol Table
 */

#ifndef TBOLC_SYMTAB_H
#define TBOLC_SYMTAB_H

#include <stdbool.h>
#include "../../shared/ast.h"

/* Symbol kinds */
typedef enum {
    SYM_VARIABLE,       /* DATA variable */
    SYM_PROCEDURE,      /* PROC definition */
    SYM_LABEL,          /* Label within a procedure */
} SymbolKind;

/* Structure group entry (for SAVE/CLEAR with structure names) */
typedef struct StructureGroup {
    char *name;                 /* Structure group name */
    int start_slot;             /* Starting RDA slot number */
    int count;                  /* Number of slots in group */
    struct StructureGroup *next;
} StructureGroup;

/* Symbol entry */
typedef struct Symbol {
    char *name;
    SymbolKind kind;
    SourceLoc defined_at;
    struct Symbol *next;    /* Hash chain */

    union {
        /* SYM_VARIABLE */
        struct {
            int array_size;     /* 0 if scalar */
            int offset;         /* Byte offset in data segment */
        } var;

        /* SYM_PROCEDURE */
        struct {
            AstNode *node;      /* Reference to AST node */
            struct Symbol *labels;  /* Labels defined in this proc */
            int order;          /* Declaration order (0 = first/main) */
        } proc;

        /* SYM_LABEL */
        struct {
            struct Symbol *proc;    /* Owning procedure */
        } label;
    } data;
} Symbol;

/* Initialize/cleanup */
void symtab_init(void);
void symtab_cleanup(void);

/* Define symbols */
Symbol *symtab_define_var(const char *name, int array_size, SourceLoc loc);
Symbol *symtab_define_proc(const char *name, AstNode *node, SourceLoc loc);
Symbol *symtab_define_label(const char *name, Symbol *proc, SourceLoc loc);

/* Lookup symbols */
Symbol *symtab_lookup_var(const char *name);
Symbol *symtab_lookup_proc(const char *name);
Symbol *symtab_lookup_label(Symbol *proc, const char *name);

/* Check if symbol exists (for duplicate detection) */
bool symtab_var_exists(const char *name);
bool symtab_proc_exists(const char *name);
bool symtab_label_exists(Symbol *proc, const char *name);

/* Structure group tracking (for SAVE/CLEAR with structure names) */
void symtab_define_structure(const char *name, int start_slot, int count);
StructureGroup *symtab_lookup_structure(const char *name);

/* Iterate every defined variable (in unspecified order). */
void symtab_foreach_var(void (*fn)(const Symbol *sym, void *ctx), void *ctx);

/* Debug */
void symtab_dump(void);

/* Each variable occupies DEFAULT_STRING_SIZE (256) bytes in the data segment.
 * VAR_SLOT converts a variable's byte offset to its RDA slot number. */
#define VAR_SLOT(sym) ((sym)->data.var.offset / 256)

#endif /* TBOLC_SYMTAB_H */
