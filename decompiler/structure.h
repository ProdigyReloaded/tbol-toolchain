/*
 * structure.h - Control flow pattern matching for decompilation
 *
 * Recognizes bytecode patterns produced by the compiler and reconstructs
 * the source-level control flow: IF/THEN, IF/THEN/ELSE, ELSE IF chains,
 * WHILE loops, compound AND/OR conditions, and GOTO.
 *
 * Compiler bytecode patterns:
 *
 * IF cond THEN stmt:       CJ(!cond) -> after; stmt;
 * IF cond THEN DO...END:   CJ(!cond) -> after; block;
 * IF/ELSE:                 CJ(!cond) -> else; then; JUMP -> end; else; end:
 * ELSE IF chain:           CJ -> else1; body; JUMP -> end; else1: CJ -> else2; ...
 * IF THEN GOTO:            CJ(!cond) -> skip; JUMP -> target; skip:
 * WHILE:                   CJ(!cond) -> exit; body; JUMP -> top;
 * AND:                     consecutive CJxx to same fail target
 * OR:                      CJ -> next; JUMP -> body; next: CJ -> next2; ...
 * (A OR B) AND C:          OR JUMPs target AND check
 * GOTO:                    JUMP -> target
 */
#ifndef STRUCTURE_H
#define STRUCTURE_H

#include "bytecode/ir.h"
#include "bytecode/proc.h"
#include "bytecode/gev.h"
#include "operand_fmt.h"
#include <stdio.h>

/*
 * Pattern mode for speculative refinement.
 * Each CJ can be emitted at different expressiveness levels.
 * The speculative loop starts at MODE_FULL and ratchets down
 * when a pattern doesn't round-trip.
 */
typedef enum {
    PMODE_FULL = 0,     /* Compound conditions + IF/ELSE + WHILE */
    PMODE_NO_ELSE,      /* IF/THEN only (no ELSE detection) */
    PMODE_FLAT,          /* IF cond THEN GOTO (always correct) */
} PatternMode;

/*
 * Per-CJ pattern mode override table.
 * Addresses not in the table default to PMODE_FULL.
 */
typedef struct {
    uint16_t *addrs;
    PatternMode *modes;
    int count;
    int capacity;
} ModeTable;

ModeTable *mode_table_new(void);
void mode_table_free(ModeTable *mt);
void mode_table_set(ModeTable *mt, uint16_t addr, PatternMode mode);
PatternMode mode_table_get(ModeTable *mt, uint16_t addr);

/*
 * Emit structured TBOL source for one procedure.
 * If mt is non-NULL, per-CJ mode overrides are applied.
 */
void emit_structured_proc(FILE *out, Program *prog, ProcBoundary *pb,
                           ProcList *procs, GEVTable *gev, DefineTable *dt,
                           StructMap *sm, ModeTable *mt, int indent);

#endif
