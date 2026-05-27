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
 * Custom YYLTYPE that includes filename
 * This allows tracking which file each token came from (critical for COPY support)
 */
#ifndef TBOL_YYLTYPE_H
#define TBOL_YYLTYPE_H

typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
    const char *filename;  /* Source file for this location */
} YYLTYPE;

#define YYLTYPE_IS_DECLARED 1
#define YYLTYPE_IS_TRIVIAL 0

#endif /* TBOL_YYLTYPE_H */
