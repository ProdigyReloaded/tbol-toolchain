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
 * TBOL Compiler - Source Debug Info (.sdb) Emission
 */

#include "sdb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* --- content hashes (staleness detection) ---
 * FNV-1a 64-bit rendered as 16 lowercase hex digits. Non-cryptographic - the
 * adapter only needs to notice that a .cod or .src changed since compile, not
 * defend against tampering. See reception-system docs/SDB-FORMAT.md. */
static void fnv1a_hex(const uint8_t *data, size_t len, char out[17]) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= data[i];
        h *= 0x100000001b3ULL;
    }
    snprintf(out, 17, "%016llx", (unsigned long long)h);
}

/* FNV-1a of a file's contents, or "-" if it cannot be read. */
static void fnv1a_file(const char *path, char out[17]) {
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(out, 17, "-"); return; }
    uint64_t h = 0xcbf29ce484222325ULL;
    int c;
    while ((c = fgetc(f)) != EOF) {
        h ^= (uint8_t)c;
        h *= 0x100000001b3ULL;
    }
    fclose(f);
    snprintf(out, 17, "%016llx", (unsigned long long)h);
}

/* --- line table --- */
typedef struct {
    int addr;               /* offset from code section start */
    int file;               /* index into file table */
    int line, col;          /* 1-based start position */
    int end_line, end_col;  /* 1-based end position */
} LineEntry;

/* --- symbol table --- */
typedef struct {
    char *name;
    char *cls;
    int   slot;
    int   len;              /* < 0 => unspecified ("-") */
} SymEntry;

/* --- procedure table --- */
typedef struct {
    char *name;
    int   start;            /* code-relative address, inclusive */
    int   end;              /* code-relative address, exclusive */
} ProcEntry;

static int         code_base = 0;

static char      **files = NULL;
static int         file_count = 0, file_cap = 0;

static LineEntry  *lines = NULL;
static int         line_count = 0, line_cap = 0;

static SymEntry   *syms = NULL;
static int         sym_count = 0, sym_cap = 0;

static ProcEntry  *procs = NULL;
static int         proc_count = 0, proc_cap = 0;

static char        cod_hash[17] = "0";  /* FNV-1a of the code section, or "0" */

void sdb_reset(void) {
    sdb_cleanup();
    code_base = 0;
    strcpy(cod_hash, "0");
}

void sdb_set_cod_bytes(const uint8_t *buf, int size) {
    /* Hash the code section only (from code_base to end) so the .sdb binds to
     * the code identity, tolerant of the volatile compile-date header. */
    if (!buf || size <= code_base) { strcpy(cod_hash, "0"); return; }
    fnv1a_hex(buf + code_base, (size_t)(size - code_base), cod_hash);
}

void sdb_set_code_base(int base) {
    code_base = base;
}

/* Intern a filename, returning its file-table index. */
static int file_index(const char *name) {
    for (int i = 0; i < file_count; i++)
        if (!strcmp(files[i], name)) return i;
    if (file_count >= file_cap) {
        file_cap = file_cap ? file_cap * 2 : 8;
        files = realloc(files, file_cap * sizeof(char *));
    }
    files[file_count] = strdup(name);
    return file_count++;
}

void sdb_mark(int file_offset, SourceRange range) {
    if (!range.start.filename || range.start.line <= 0) return;

    int addr = file_offset - code_base;
    if (addr < 0) return;

    LineEntry e;
    e.addr     = addr;
    e.file     = file_index(range.start.filename);
    e.line     = range.start.line;
    e.col      = range.start.column;
    e.end_line = range.end.line > 0 ? range.end.line : range.start.line;
    e.end_col  = range.end.line > 0 ? range.end.column : range.start.column;

    /* A statement and any labels/definitions preceding it can land on the same
     * address (the label emits no code). Keep the most recent mark so the
     * innermost / real instruction wins. */
    if (line_count > 0 && lines[line_count - 1].addr == addr) {
        lines[line_count - 1] = e;
        return;
    }

    if (line_count >= line_cap) {
        line_cap = line_cap ? line_cap * 2 : 64;
        lines = realloc(lines, line_cap * sizeof(LineEntry));
    }
    lines[line_count++] = e;
}

void sdb_add_symbol(const char *name, const char *cls, int slot, int len) {
    if (sym_count >= sym_cap) {
        sym_cap = sym_cap ? sym_cap * 2 : 32;
        syms = realloc(syms, sym_cap * sizeof(SymEntry));
    }
    syms[sym_count].name = strdup(name);
    syms[sym_count].cls  = strdup(cls);
    syms[sym_count].slot = slot;
    syms[sym_count].len  = len;
    sym_count++;
}

void sdb_add_proc(const char *name, int start_off, int end_off) {
    int start = start_off - code_base;
    int end   = end_off - code_base;
    if (start < 0) start = 0;
    if (end < start) end = start;

    if (proc_count >= proc_cap) {
        proc_cap = proc_cap ? proc_cap * 2 : 16;
        procs = realloc(procs, proc_cap * sizeof(ProcEntry));
    }
    procs[proc_count].name  = strdup(name);
    procs[proc_count].start = start;
    procs[proc_count].end   = end;
    proc_count++;
}

/* Order symbols by class, then by slot ascending (stable-ish for equal keys). */
static int sym_cmp(const void *a, const void *b) {
    const SymEntry *x = a, *y = b;
    int c = strcmp(x->cls, y->cls);
    if (c) return c;
    return x->slot < y->slot ? -1 : x->slot > y->slot ? 1 : 0;
}

int sdb_write(const char *path, const char *program, const char *cod_name) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "SDB 1\n");
    fprintf(f, "program %s TBOL\n", program ? program : "?");
    fprintf(f, "cod %s %s\n", cod_name ? cod_name : "?", cod_hash);

    /* Record each source as an absolute path so a debugger can open it directly
     * (DAP source paths must be absolute), independent of build/output layout.
     * Falls back to the recorded name if realpath fails. The trailing hash of
     * the file's contents lets the adapter warn when source drifted from the
     * compiled line table (edited since compile). */
    fprintf(f, "\n[files]\n");
    for (int i = 0; i < file_count; i++) {
        char *abs = realpath(files[i], NULL);
        char h[17];
        fnv1a_file(abs ? abs : files[i], h);
        fprintf(f, "%d %s %s\n", i, abs ? abs : files[i], h);
        free(abs);
    }

    fprintf(f, "\n[lines]\n");
    for (int i = 0; i < line_count; i++) {
        const LineEntry *e = &lines[i];
        fprintf(f, "0x%04X  %d  %d  %d  %d  %d\n",
                (unsigned)(e->addr & 0xFFFF),
                e->file, e->line, e->col, e->end_line, e->end_col);
    }

    /* Procedures, half-open [addr, endAddr) code ranges, ascending by address
     * (emitted in definition order, which is already ascending). Lets the
     * adapter map a PC / return address to a function and build frames. */
    if (proc_count > 0) {
        fprintf(f, "\n[procs]\n");
        for (int i = 0; i < proc_count; i++) {
            fprintf(f, "%s  0x%04X  0x%04X\n",
                    procs[i].name,
                    (unsigned)(procs[i].start & 0xFFFF),
                    (unsigned)(procs[i].end & 0xFFFF));
        }
    }

    /* Emit symbols grouped by class, ascending by slot -- source order is the
     * preprocessor's hash-iteration order, which is meaningless to a reader. */
    qsort(syms, sym_count, sizeof(SymEntry), sym_cmp);

    fprintf(f, "\n[symbols]\n");
    for (int i = 0; i < sym_count; i++) {
        const SymEntry *s = &syms[i];
        if (s->len >= 0)
            fprintf(f, "%s  %s  %d  %d\n", s->name, s->cls, s->slot, s->len);
        else
            fprintf(f, "%s  %s  %d  -\n", s->name, s->cls, s->slot);
    }

    fclose(f);
    return 0;
}

void sdb_cleanup(void) {
    for (int i = 0; i < file_count; i++) free(files[i]);
    free(files);
    files = NULL; file_count = file_cap = 0;

    free(lines);
    lines = NULL; line_count = line_cap = 0;

    for (int i = 0; i < sym_count; i++) { free(syms[i].name); free(syms[i].cls); }
    free(syms);
    syms = NULL; sym_count = sym_cap = 0;

    for (int i = 0; i < proc_count; i++) free(procs[i].name);
    free(procs);
    procs = NULL; proc_count = proc_cap = 0;
}
