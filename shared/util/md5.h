/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
 * see <https://www.gnu.org/licenses/>.
 *
 *
 * md5.h - small portable MD5 implementation
 *
 * Per RFC 1321. Used for stamping decompiled / disassembled output with
 * the input file's checksum so we can later trace what each emitted
 * source artifact came from.  Not used for any security purpose.
 */
#ifndef TBOL_UTIL_MD5_H
#define TBOL_UTIL_MD5_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t state[4];
    uint64_t count;     /* total bytes processed */
    uint8_t  buffer[64];
} MD5Ctx;

void md5_init(MD5Ctx *ctx);
void md5_update(MD5Ctx *ctx, const void *data, size_t len);
void md5_finalize(MD5Ctx *ctx, uint8_t digest[16]);

/* One-shot helpers */
void md5_buffer(const void *data, size_t len, uint8_t digest[16]);
void md5_hex(const uint8_t digest[16], char out[33]);

#endif
