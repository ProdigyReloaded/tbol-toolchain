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
 * strutil.h - Small portable string helpers for the few GNU/BSD extensions
 * absent from MSYS2 UCRT64.  POSIX targets get the system implementation
 * via <string.h> / <strings.h>; Windows gets a static-inline fallback.
 */
#ifndef TBOL_UTIL_STRUTIL_H
#define TBOL_UTIL_STRUTIL_H

#ifdef _WIN32

#include <ctype.h>
#include <string.h>

/* Case-insensitive substring search.  MinGW-w64 UCRT64 does not provide
 * strcasestr (a GNU extension); supply a small inline equivalent. */
static inline char *strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    size_t nlen = strlen(needle);
    for (const char *h = haystack; *h; h++) {
        size_t i;
        for (i = 0; i < nlen; i++) {
            if (tolower((unsigned char)h[i]) != tolower((unsigned char)needle[i])) break;
        }
        if (i == nlen) return (char *)h;
    }
    return NULL;
}

#endif /* _WIN32 */

#endif /* TBOL_UTIL_STRUTIL_H */
