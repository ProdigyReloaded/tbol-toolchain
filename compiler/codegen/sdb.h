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
 *
 * Collects an address->source-span line table and a symbol table during code
 * generation, then writes a `.sdb` file (see reception-system docs/SDB-FORMAT.md).
 * Addresses in the line table are offsets from the code section start, matching
 * the `address` a bytecode loader assigns when decoding the `.cod` code section.
 */

#ifndef TBOLC_SDB_H
#define TBOLC_SDB_H

#include <stdint.h>
#include "../../shared/ast.h"

/* Reset all collected state (call once per program, before codegen). */
void sdb_reset(void);

/* Record the byte offset where the code section begins. Line-table addresses
 * are computed relative to this base. */
void sdb_set_code_base(int base);

/* Record the emitted .cod buffer (whole file). The code section (from the
 * code base to `size`) is hashed for the `cod` staleness field. Call after the
 * code base is set and the buffer is final. */
void sdb_set_cod_bytes(const uint8_t *buf, int size);

/* Record a line-table entry: the instruction at `file_offset` maps to `range`.
 * Ignored when the range carries no source position. If an entry already exists
 * at the same address it is overwritten (the most specific/innermost mark wins). */
void sdb_mark(int file_offset, SourceRange range);

/* Record a symbol. `cls` is an SDB storage class ("RDA", "PEV", ...). A negative
 * `len` is written as "-" (unspecified). */
void sdb_add_symbol(const char *name, const char *cls, int slot, int len);

/* Record a procedure and its half-open code-address range [start_off, end_off),
 * given as absolute file offsets (converted to code-section-relative addresses
 * like sdb_mark). Lets a debugger map a PC / return address to a function name
 * and build a call stack. */
void sdb_add_proc(const char *name, int start_off, int end_off);

/* Write the collected info to `path`. `program` is the program name, `cod_name`
 * the emitted `.cod` filename. Returns 0 on success, non-zero on I/O error. */
int sdb_write(const char *path, const char *program, const char *cod_name);

/* Free collected state. */
void sdb_cleanup(void);

#endif /* TBOLC_SDB_H */
