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
#include "cod_file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
 * Read 16-bit big-endian value
 */
static uint16_t read_be16(const uint8_t *p) {
    return (p[0] << 8) | p[1];
}

/*
 * Read 16-bit little-endian value
 */
static uint16_t read_le16(const uint8_t *p) {
    return p[0] | (p[1] << 8);
}

/*
 * Detect Prodigy object encapsulation (PGM/PDO files).
 * Returns offset to COD data within the first Program Data segment,
 * or 0 if not an encapsulated object.
 *
 * Prodigy object structure (per ADRM):
 *   Object Header (18 bytes):
 *     0-12:  Object ID (13 bytes, e.g. "3COPCHO1PGM\x00\x0C")
 *     13-14: Object length (16-bit LE)
 *     15-17: Control (3 bytes: store flags, set size, version)
 *   Segments (one or more):
 *     0:     Segment type
 *     1-2:   Segment length N (16-bit LE)
 *     3..N+2: Segment payload
 *
 * For PDO (Program Data Object, object type 0x0C):
 *   Segment type 0x61 = Program Data
 *   First payload byte = subtype (0x01 = compiled TBOL, 0x02 = app data)
 *   Remaining payload bytes = COD file contents
 */
static int detect_pgm_offset(const uint8_t *data, long size, long *cod_size_out) {
    if (size < 22) return 0;

    /* Check for PDO: Object ID byte 12 must be object type 0x0C */
    if (data[12] != 0x0C) return 0;

    /* Verify Object ID bytes 0-11 look like a Prodigy object name:
     * typically printable chars with possible trailing control bytes */
    bool has_printable = false;
    for (int i = 0; i < 8; i++) {
        if (isprint(data[i]) && data[i] != ' ') {
            has_printable = true;
        }
    }
    if (!has_printable) return 0;

    /* Walk segments starting at byte 18 to find Program Data (0x61) */
    int pos = 18;
    while (pos + 3 <= size) {
        uint8_t seg_type = data[pos];
        uint16_t seg_len = read_le16(data + pos + 1);

        if (seg_type == 0x61 && seg_len >= 1) {
            uint8_t subtype = data[pos + 3];
            if (subtype == 0x01) {
                /* COD data starts after segment header + subtype byte */
                if (cod_size_out) {
                    *cod_size_out = seg_len - 1; /* subtract subtype byte */
                }
                return pos + 3 + 1;
            }
        }

        pos += 3 + seg_len;
    }

    return 0;
}

/*
 * Detect short COD header format
 *
 * Short header format (4 bytes):
 *   0-1: File size (16-bit BE, inclusive of these bytes)
 *   2-3: Code start offset (16-bit BE, from beginning of file)
 *   4:   0xFF marker indicates short header
 *
 * Returns true if this is a short header format
 */
static bool detect_short_header(const uint8_t *data, long size) {
    if (size < 5) return false;
    /* If byte 4 is 0xFF, it's likely a short header */
    return data[4] == 0xFF;
}

/*
 * Load a .cod or .pgm file
 *
 * Long COD Header format:
 *   0-1: File size (16-bit BE)
 *   2-3: Code start offset (16-bit BE)
 *   4-5: Name length (16-bit BE)
 *   6+:  Program name (name_len bytes)
 *   +14: Date/time "MM/DD/YY HH:MM"
 *   +5:  Version "04.21"
 *   +1:  0x30 marker
 *   [Code section starts at code_start]
 *
 * Short COD Header format:
 *   0-1: File size (16-bit BE, inclusive)
 *   2-3: Code start offset (16-bit BE)
 *   4:   0xFF marker
 *   [Code section starts at code_start]
 *
 * PGM files are Prodigy objects with an 18-byte header + segment structure
 */
Program *cod_file_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return NULL;
    }

    /* Get file size */
    fseek(f, 0, SEEK_END);
    long actual_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Read entire file */
    uint8_t *data = malloc(actual_size);
    if (!data) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return NULL;
    }
    if (fread(data, 1, actual_size, f) != (size_t)actual_size) {
        fprintf(stderr, "Error: failed to read file\n");
        free(data);
        fclose(f);
        return NULL;
    }
    fclose(f);

    /* Detect Prodigy object encapsulation and find COD data */
    long pgm_cod_size = 0;
    int pgm_offset = detect_pgm_offset(data, actual_size, &pgm_cod_size);
    uint8_t *cod_data = data + pgm_offset;
    long cod_size = pgm_offset ? pgm_cod_size : actual_size;

    /* Parse header - detect short vs long format */
    if (cod_size < 5) {
        fprintf(stderr, "Error: file too small for header\n");
        free(data);
        return NULL;
    }

    uint16_t file_size = read_be16(cod_data);
    uint16_t code_start = read_be16(cod_data + 2);

    /* Create program structure */
    Program *prog = program_new();
    if (!prog) {
        free(data);
        return NULL;
    }

    if (detect_short_header(cod_data, cod_size)) {
        /* Short header format - no name/date/version */
        if (code_start > cod_size) {
            fprintf(stderr, "Error: invalid short header (code_start=%d, cod_size=%ld)\n",
                    code_start, cod_size);
            program_free(prog);
            free(data);
            return NULL;
        }

        /* Extract filename from path for program name */
        const char *basename = strrchr(path, '/');
        basename = basename ? basename + 1 : path;
        const char *dot = strrchr(basename, '.');
        size_t name_len = dot ? (size_t)(dot - basename) : strlen(basename);
        prog->program_name = malloc(name_len + 1);
        memcpy(prog->program_name, basename, name_len);
        prog->program_name[name_len] = '\0';

        prog->date_time = strdup("unknown");
        prog->version = strdup("unknown");

        /* Store code section - use file_size from header to exclude padding */
        prog->code_start = code_start;
        prog->code_size = (file_size > code_start) ? file_size - code_start
                                                    : cod_size - code_start;
        prog->code = malloc(prog->code_size);
        memcpy(prog->code, cod_data + code_start, prog->code_size);
    } else {
        /* Long header format */
        if (cod_size < 6) {
            fprintf(stderr, "Error: file too small for long header\n");
            program_free(prog);
            free(data);
            return NULL;
        }

        uint16_t name_len = read_be16(cod_data + 4);

        /* Validate */
        if (code_start > cod_size || 6 + name_len + 20 > code_start) {
            fprintf(stderr, "Error: invalid header (code_start=%d, name_len=%d, cod_size=%ld)\n",
                    code_start, name_len, cod_size);
            program_free(prog);
            free(data);
            return NULL;
        }

        /* Extract program name */
        prog->program_name = malloc(name_len + 1);
        memcpy(prog->program_name, cod_data + 6, name_len);
        prog->program_name[name_len] = '\0';

        /* Extract date/time "MM/DD/YY HH:MM" (14 chars after name) */
        prog->date_time = malloc(15);
        memcpy(prog->date_time, cod_data + 6 + name_len, 14);
        prog->date_time[14] = '\0';

        /* Extract version "04.21" (5 chars after date); a 0x30 marker follows */
        prog->version = malloc(6);
        memcpy(prog->version, cod_data + 6 + name_len + 14, 5);
        prog->version[5] = '\0';

        /* Store code section - use file_size from header to exclude padding */
        prog->code_start = code_start;
        prog->code_size = (file_size > code_start) ? file_size - code_start
                                                    : cod_size - code_start;
        prog->code = malloc(prog->code_size);
        memcpy(prog->code, cod_data + code_start, prog->code_size);
    }

    free(data);
    return prog;
}
