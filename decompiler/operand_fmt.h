/*
 * operand_fmt.h — Operand formatting for source emission
 */
#ifndef OPERAND_FMT_H
#define OPERAND_FMT_H

#include "bytecode/ir.h"
#include "bytecode/gev.h"
#include <stdio.h>

/* ── Define table for non-printable string constants ────────────────── */

typedef struct {
    char *hex_value;     /* "0x474254..." — the hex literal */
    char *ident;         /* "gbtsnext_pgm" — the generated identifier */
    int raw_len;         /* length of raw bytes */
    char *raw_bytes;     /* raw byte content for matching */
} DefineEntry;

typedef struct {
    DefineEntry *entries;
    int count;
    int capacity;
} DefineTable;

DefineTable *define_table_new(void);
void define_table_free(DefineTable *dt);

/* Scan all instructions and collect non-printable string operands into defines.
 * Call before emission. */
void define_table_scan(DefineTable *dt, Program *prog);

/* Look up a non-printable string operand; returns the DEFINE identifier
 * or NULL if not in the table. */
const char *define_table_lookup(DefineTable *dt, Operand *op);

/* ── Struct map for CLEAR/SAVE range names ──────────────────────────── */

typedef struct {
    int start_slot;
    int count;
    char name[16];       /* e.g., "dt" or "st_59" */
} StructEntry;

typedef struct {
    StructEntry *entries;
    int count;
    int capacity;
} StructMap;

StructMap *struct_map_new(void);
void struct_map_free(StructMap *sm);
void struct_map_add(StructMap *sm, int start_slot, int count, const char *name);

/* Look up the struct name for a CLEAR/SAVE target slot. Returns NULL if not a struct. */
const char *struct_map_lookup(StructMap *sm, int slot);

/* ── Operand formatting ─────────────────────────────────────────────── */

/* Format a single operand as TBOL source text. Writes to buf.
 * If dt is non-NULL, non-printable strings are resolved to DEFINE identifiers.
 * Returns the number of characters written. */
int fmt_operand(char *buf, int bufsize, Operand *op, GEVTable *gev, DefineTable *dt);

/* Format a condition (CJxx) as a TBOL source expression.
 * Inverts the condition since CJxx means "jump if condition FALSE".
 * Writes e.g. "RDA1 = '5'" or "I3 <> RDA10" */
int fmt_condition(char *buf, int bufsize, Instruction *instr, GEVTable *gev);

/* Format an instruction's mnemonic name as it should appear in source */
const char *fmt_mnemonic(Instruction *instr);

#endif
