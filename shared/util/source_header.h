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
 * source_header.h - shared comment-block emitter for tboldasm and
 * tboldc.  Both tools open their output with the same four-line block:
 *
 *   { Program 'NAME' compiled with TBOL COMPILER version X.YYY }
 *   { Date Program compiled MM/DD/YY HH:MM:SS }
 *   { Original file BASENAME }
 *   { md5sum HEX }
 *
 * The first two come from the bytecode's compilation header.  The last
 * two are computed from the on-disk input so the emitted source carries
 * a verifiable pointer back to where it came from.
 */
#ifndef TBOL_UTIL_SOURCE_HEADER_H
#define TBOL_UTIL_SOURCE_HEADER_H

#include <stdio.h>
#include "bytecode/ir.h"

/*
 * Emit the source-header comment block to `out`.
 *
 *   prog        - decoded program (for name / version / date_time)
 *   input_path  - path to the .COD/.PGM that produced `prog`; used for
 *                 the basename and md5sum lines.  May be NULL, in which
 *                 case those two lines are omitted.
 *   prefix      - per-line prefix (used by tboldasm's tabular mode to
 *                 indent comments past the bytes column).  May be NULL.
 */
void emit_source_header(FILE *out, const Program *prog,
                        const char *input_path, const char *prefix);

#endif
