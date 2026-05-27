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
 */
#include "memfile.h"

#include <stdlib.h>
#include <string.h>

#ifndef _WIN32

/* POSIX: thin wrappers around open_memstream / fmemopen. */

FILE *mem_fopen_growable(char **bufp, size_t *sizep) {
    return open_memstream(bufp, sizep);
}

void mem_fclose_growable(FILE *fp, char **bufp, size_t *sizep) {
    (void)bufp;
    (void)sizep;
    if (fp) fclose(fp);  /* open_memstream finalizes buf/size at close */
}

FILE *mem_fopen_fixed(char *buf, size_t size) {
    return fmemopen(buf, size, "w");
}

void mem_fclose_fixed(FILE *fp, char *buf, size_t size) {
    (void)buf;
    (void)size;
    if (fp) fclose(fp);  /* fmemopen wrote into buf during fprintf */
}

#else /* _WIN32 */

/* Windows: stdlib has no open_memstream / fmemopen.  Round-trip through
 * tmpfile() + a read-back at close.  Slower than memory-only, but the
 * buffers we use here are small (KB scale) and these helpers are not on
 * any perf-critical path. */

FILE *mem_fopen_growable(char **bufp, size_t *sizep) {
    if (bufp)  *bufp = NULL;
    if (sizep) *sizep = 0;
    return tmpfile();
}

void mem_fclose_growable(FILE *fp, char **bufp, size_t *sizep) {
    if (!fp) {
        if (bufp)  *bufp = NULL;
        if (sizep) *sizep = 0;
        return;
    }
    fflush(fp);
    fseek(fp, 0, SEEK_END);
    long n = ftell(fp);
    if (n < 0) n = 0;
    char *buf = (char *)malloc((size_t)n + 1);
    size_t got = 0;
    if (buf) {
        fseek(fp, 0, SEEK_SET);
        got = fread(buf, 1, (size_t)n, fp);
        buf[got] = '\0';
    }
    fclose(fp);
    if (bufp)  *bufp  = buf;
    if (sizep) *sizep = got;
}

FILE *mem_fopen_fixed(char *buf, size_t size) {
    if (buf && size) buf[0] = '\0';
    return tmpfile();
}

void mem_fclose_fixed(FILE *fp, char *buf, size_t size) {
    if (!fp) return;
    fflush(fp);
    fseek(fp, 0, SEEK_SET);
    size_t cap = (size > 0) ? size - 1 : 0;
    size_t got = (buf && cap) ? fread(buf, 1, cap, fp) : 0;
    if (buf && size > 0) buf[got] = '\0';
    fclose(fp);
}

#endif /* _WIN32 */
