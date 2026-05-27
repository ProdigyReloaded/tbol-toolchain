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
 * TBOL Compiler - Bytecode Emission Utilities
 */

#ifndef TBOLC_EMIT_H
#define TBOLC_EMIT_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Initialize/cleanup output buffer */
void emit_init(void);
void emit_cleanup(void);

/* Get current code position (for labels) */
int emit_get_offset(void);

/* Emit raw bytes */
void emit_byte(uint8_t b);
void emit_word_be(uint16_t w);  /* Big-endian 16-bit */
void emit_bytes(const uint8_t *data, int len);

/* Emit string (length-prefixed) */
void emit_string(const char *str);
void emit_string_n(const char *str, int len);  /* Explicit length (for embedded nulls) */

/* Forward reference management */
typedef struct {
    int patch_offset;       /* Where to patch */
    const char *label;      /* Label name */
    const char *proc;       /* Procedure name (for scoping) */
} ForwardRef;

void emit_add_forward_ref(int offset, const char *label, const char *proc);
int emit_get_forward_ref_count(void);
ForwardRef *emit_get_forward_refs(void);
void emit_clear_forward_refs(void);

/* Patch values at a given position */
void emit_patch_byte(int offset, uint8_t value);
void emit_patch_word_be(int offset, int16_t value);

/* Write output to file */
int emit_write_to_file(FILE *fp);

/* Get buffer for inspection */
const uint8_t *emit_get_buffer(int *size);

#endif /* TBOLC_EMIT_H */
