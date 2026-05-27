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
 */
#include "source_header.h"
#include "md5.h"
#include <stdlib.h>
#include <string.h>

static void emit_md5_line(FILE *out, const char *prefix, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return; }
    rewind(f);

    uint8_t *data = malloc(sz > 0 ? (size_t)sz : 1);
    if (!data) { fclose(f); return; }

    size_t got = fread(data, 1, (size_t)sz, f);
    fclose(f);

    uint8_t digest[16];
    char hex[33];
    md5_buffer(data, got, digest);
    md5_hex(digest, hex);
    fprintf(out, "%s{ md5sum %s }\n", prefix, hex);

    free(data);
}

void emit_source_header(FILE *out, const Program *prog,
                        const char *input_path, const char *prefix) {
    if (!prefix) prefix = "";

    fprintf(out, "%s{ Program '%s' compiled with TBOL COMPILER version %s }\n",
            prefix,
            prog && prog->program_name ? prog->program_name : "?",
            prog && prog->version      ? prog->version      : "?");
    fprintf(out, "%s{ Date Program compiled %s }\n",
            prefix,
            prog && prog->date_time ? prog->date_time : "unknown");

    if (input_path) {
        const char *slash = strrchr(input_path, '/');
        const char *base = slash ? slash + 1 : input_path;
        fprintf(out, "%s{ Original file %s }\n", prefix, base);
        emit_md5_line(out, prefix, input_path);
    }
}
