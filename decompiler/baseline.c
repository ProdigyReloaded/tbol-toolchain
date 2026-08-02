/*
 * baseline.c - Top-level source emission
 *
 * Emits the PROGRAM header, COPY directives, DATA section, and
 * procedure structure. Delegates instruction-level emission to
 * structure.c for control flow pattern matching.
 */

#include "baseline.h"
#include "structure.h"
#include "operand_fmt.h"
#include "tbol_fmt.h"
#include "bytecode/opcodes.h"
#include "bytecode/symtab_analysis.h"
#include "source_header.h"
#include "memfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -- DATA section emission --------------------------------------------- */

static void emit_data_section(FILE *out, Program *prog, StructMap *sm) {
    SymbolTable *st = symbol_table_new();
    symbol_table_scan(st, prog);

    if (st->max_slot < 0) {
        symbol_table_free(st);
        return;
    }

    /* Register confirmed struct ranges in the StructMap */
    for (int r = 0; r < st->range_count; r++) {
        if (!st->ranges[r].is_confirmed_struct || st->ranges[r].count <= 1)
            continue;
        char name[16];
        snprintf(name, sizeof(name), "st_%d", st->ranges[r].start_slot);
        struct_map_add(sm, st->ranges[r].start_slot, st->ranges[r].count, name);
    }

    /* Emit DATA section. All slots appear in order. Struct ranges become
     * named groups; everything else goes in "vars". Groups must be
     * emitted in slot order so the compiler assigns sequential slots. */
    fprintf(out, "DATA\n");

    /* State machine: we're either emitting into "vars" or a struct group.
     * When we hit a struct start, close vars, open the struct group.
     * When the struct ends, reopen vars. */
    bool in_vars = false;
    bool first_in_group = true;
    int i = 0;

    while (i <= st->max_slot) {
        /* Check if a struct group starts here */
        const char *sname = struct_map_lookup(sm, i);
        if (sname) {
            /* Close vars if open */
            if (in_vars) {
                fprintf(out, ";\n");
                in_vars = false;
            }
            /* Emit struct group */
            int scount = 0;
            for (int s = 0; s < sm->count; s++)
                if (sm->entries[s].start_slot == i) { scount = sm->entries[s].count; break; }
            fprintf(out, "  %s =\n", sname);
            for (int j = 0; j < scount; j++) {
                fprintf(out, "    RDA%d", i + j);
                if (j + 1 < scount) fprintf(out, ",");
                fprintf(out, "\n");
            }
            fprintf(out, ";\n");
            i += scount;
            first_in_group = true;
            continue;
        }

        /* Regular slot - goes into vars */
        if (!in_vars) {
            fprintf(out, "  vars =\n");
            in_vars = true;
            first_in_group = true;
        }

        if (!first_in_group) fprintf(out, ",\n");
        first_in_group = false;

        /* Check for non-struct array range (CLEAR/SAVE) */
        int range_dim = 0;
        for (int r = 0; r < st->range_count; r++) {
            if (st->ranges[r].start_slot == i && st->ranges[r].count > 1 &&
                !st->ranges[r].is_confirmed_struct) {
                range_dim = st->ranges[r].count;
                break;
            }
        }

        if (range_dim > 0) {
            fprintf(out, "    RDA%d(%d)", i, range_dim);
            i += range_dim;
        } else if (st->slots[i].is_array && st->slots[i].max_index > 0) {
            int dim = st->slots[i].max_index;
            fprintf(out, "    RDA%d(%d)", i, dim);
            i += dim;
        } else {
            fprintf(out, "    RDA%d", i);
            i++;
        }
    }

    if (in_vars) fprintf(out, ";\n");
    fprintf(out, "\n");
    symbol_table_free(st);
}

/* -- DEFINE emission for non-printable string constants ---------------- */

static void emit_defines(FILE *out, DefineTable *dt) {
    if (!dt || dt->count == 0) return;
    for (int i = 0; i < dt->count; i++) {
        fprintf(out, "DEFINE %s, %s;\n", dt->entries[i].ident, dt->entries[i].hex_value);
    }
    fprintf(out, "\n");
}

/* -- Main emission ----------------------------------------------------- */

void emit_baseline(FILE *out, Program *prog, ProcList *procs, GEVTable *gev,
                   ModeTable *mt, const char *input_path) {
    /* Emit to memory buffer first, then strip unreferenced labels */
    char *buf = NULL;
    size_t buf_size = 0;
    FILE *mem = mem_fopen_growable(&buf, &buf_size);

    /* Source-header comment block (matches what tboldasm emits, with the
     * additional "Original file" / "md5sum" lines). */
    emit_source_header(mem, prog, input_path, NULL);
    fputs("\n", mem);

    /* Header */
    fprintf(mem, "PROGRAM %s;\n\n", prog->program_name);

    /* COPY XXCGTSYS if any instruction references GEV symbols, uses
     * SET_FUNCTION/TRIGGER_FUNCTION action names, or compares
     * SYS_RETURN_CODE against a literal (resolved to RET_* names). */
    bool needs_xxcgtsys = false;
    for (Instruction *i = prog->instructions; i && !needs_xxcgtsys; i = i->next) {
        for (int j = 0; j < i->operand_count; j++)
            if (i->operands[j].kind == OP_GEV) { needs_xxcgtsys = true; break; }
        if (gev && (i->mnemonic == MNEM_SET_FUNCTION || i->opcode == OP_TRIG_FUNC) &&
            i->operand_count >= 1)
            needs_xxcgtsys = true;
    }
    if (needs_xxcgtsys)
        fprintf(mem, "COPY XXCGTSYS;\n\n");

    /* DATA section + struct map */
    StructMap *sm = struct_map_new();
    emit_data_section(mem, prog, sm);

    /* Scan for non-printable string constants -> DEFINE table */
    DefineTable *dt = define_table_new();
    define_table_scan(dt, prog);
    emit_defines(mem, dt);

    /* Procedures */
    for (int p = 0; p < procs->count; p++) {
        ProcBoundary *pb = &procs->procs[p];

        if (pb->is_main)
            fprintf(mem, "PROC main =\n");
        else
            fprintf(mem, "PROC proc_%d =\n", pb->proc_num);

        emit_structured_proc(mem, prog, pb, procs, gev, dt, sm, mt, 2);

        fprintf(mem, "END_PROC\n\n");
    }

    mem_fclose_growable(mem, &buf, &buf_size);

    /* Format: strip unreferenced labels, cuddle ELSE, normalize indentation */
    TbolFmtOptions fmt_opts = tbol_fmt_defaults();
    char *formatted = tbol_fmt(buf, &fmt_opts);
    free(buf);
    fputs(formatted, out);
    free(formatted);
    define_table_free(dt);
    struct_map_free(sm);
}
