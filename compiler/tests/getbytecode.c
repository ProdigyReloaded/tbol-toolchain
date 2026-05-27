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
 * getbytecode - Extract raw bytecode from TBOL .COD or .PGM files
 *
 * Usage: getbytecode <file.cod|file.pgm>
 *
 * Strips all encapsulation (PGM wrapper, COD header) and outputs
 * only the raw bytecode to stdout.
 *
 * Useful for bytecode comparison without worrying about headers/timestamps.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/*
 * Read 16-bit big-endian value
 */
static uint16_t read_be16(const uint8_t *p) {
    return (p[0] << 8) | p[1];
}

/*
 * Detect PGM encapsulation format
 * PGM files start with program name (1-8 printable chars) followed by "PGM\0"
 * Returns offset to COD data, or 0 if not a PGM file
 */
static int detect_pgm_offset(const uint8_t *data, long size) {
    if (size < 22) return 0;

    /* Look for "PGM\0" after a program name (1-8 printable chars) */
    for (int i = 1; i <= 8 && i + 4 <= size; i++) {
        if (data[i] == 'P' && data[i+1] == 'G' && data[i+2] == 'M' && data[i+3] == '\0') {
            /* Verify preceding bytes are printable (program name) */
            bool valid = true;
            for (int j = 0; j < i; j++) {
                if (!isprint(data[j]) || data[j] == ' ') {
                    valid = false;
                    break;
                }
            }
            if (valid) {
                /* PGM header is 22 bytes before COD data */
                return 22;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: getbytecode <file.cod|file.pgm>\n");
        fprintf(stderr, "\nExtracts raw bytecode, stripping all headers.\n");
        fprintf(stderr, "Output is written to stdout (binary).\n");
        return 1;
    }

    const char *path = argv[1];

    /* Open and read file */
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "Error: cannot open '%s'\n", path);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long actual_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = malloc(actual_size);
    if (!data) {
        fprintf(stderr, "Error: out of memory\n");
        fclose(f);
        return 1;
    }

    if (fread(data, 1, actual_size, f) != (size_t)actual_size) {
        fprintf(stderr, "Error: failed to read file\n");
        free(data);
        fclose(f);
        return 1;
    }
    fclose(f);

    /* Detect PGM encapsulation and find COD data offset */
    int pgm_offset = detect_pgm_offset(data, actual_size);
    uint8_t *cod_data = data + pgm_offset;
    long cod_size = actual_size - pgm_offset;

    /* Parse COD header */
    if (cod_size < 6) {
        fprintf(stderr, "Error: file too small for COD header\n");
        free(data);
        return 1;
    }

    uint16_t code_start = read_be16(cod_data + 2);

    /* Validate */
    if (code_start > cod_size) {
        fprintf(stderr, "Error: invalid header (code_start=%d > cod_size=%ld)\n",
                code_start, cod_size);
        free(data);
        return 1;
    }

    /* Output raw bytecode */
    long bytecode_size = cod_size - code_start;
    uint8_t *bytecode = cod_data + code_start;

    if (fwrite(bytecode, 1, bytecode_size, stdout) != (size_t)bytecode_size) {
        fprintf(stderr, "Error: failed to write output\n");
        free(data);
        return 1;
    }

    free(data);
    return 0;
}
