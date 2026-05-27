/*
 * Copyright 2025-2026, Phillip Heller
 *
 * This file is part of Prodigy Reloaded.
 *
 * Prodigy Reloaded is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Prodigy Reloaded is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Prodigy Reloaded.  If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * memfile.h - portable in-memory FILE* streams.
 *
 * Both POSIX 2008 (open_memstream, fmemopen) and Windows are supported.
 * On POSIX these are thin wrappers; on Windows the helpers round-trip
 * through tmpfile() + a read-back at close.  The two-call shape
 * (open + close) is the same on both platforms so callers do not need
 * an #ifdef.
 */
#ifndef TBOL_UTIL_MEMFILE_H
#define TBOL_UTIL_MEMFILE_H

#include <stdio.h>
#include <stddef.h>

/* Open a writable in-memory FILE* whose contents are exposed as a
 * malloc'd, NUL-terminated buffer via *bufp / *sizep when the stream
 * is closed.  Caller frees *bufp.  Returns NULL on failure. */
FILE *mem_fopen_growable(char **bufp, size_t *sizep);

/* Close a stream opened by mem_fopen_growable.  On return, *bufp points
 * to a malloc'd, NUL-terminated buffer holding all bytes written; *sizep
 * is the byte count (excluding the trailing NUL). */
void  mem_fclose_growable(FILE *fp, char **bufp, size_t *sizep);

/* Open a writable in-memory FILE* backed by the caller-owned fixed
 * buffer `buf` of `size` bytes.  At most size-1 bytes can be written
 * (one byte is reserved for the trailing NUL written at close). */
FILE *mem_fopen_fixed(char *buf, size_t size);

/* Close a stream opened by mem_fopen_fixed.  On return, buf is
 * NUL-terminated within [0, size). */
void  mem_fclose_fixed(FILE *fp, char *buf, size_t size);

#endif /* TBOL_UTIL_MEMFILE_H */
