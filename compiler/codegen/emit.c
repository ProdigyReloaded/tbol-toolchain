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
 * TBOL Compiler - Bytecode Emission Utilities Implementation
 */

#include "emit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Output buffer */
#define INITIAL_BUFFER_SIZE 4096
static uint8_t *buffer = NULL;
static int buffer_size = 0;
static int buffer_capacity = 0;

/* Forward references */
static ForwardRef *forward_refs = NULL;
static int forward_ref_count = 0;
static int forward_ref_capacity = 0;

void emit_init(void) {
    buffer = malloc(INITIAL_BUFFER_SIZE);
    buffer_size = 0;
    buffer_capacity = INITIAL_BUFFER_SIZE;
    forward_ref_count = 0;
}

void emit_cleanup(void) {
    emit_clear_forward_refs();
    free(buffer);
    buffer = NULL;
    buffer_size = 0;
    buffer_capacity = 0;
}

static void ensure_capacity(int need) {
    if (buffer_size + need > buffer_capacity) {
        while (buffer_size + need > buffer_capacity) {
            buffer_capacity *= 2;
        }
        uint8_t *tmp = realloc(buffer, buffer_capacity);
        if (!tmp) {
            fprintf(stderr, "out of memory\n");
            exit(1);
        }
        buffer = tmp;
    }
}

int emit_get_offset(void) {
    return buffer_size;
}

void emit_byte(uint8_t b) {
    ensure_capacity(1);
    buffer[buffer_size++] = b;
}

void emit_word_be(uint16_t w) {
    ensure_capacity(2);
    buffer[buffer_size++] = (w >> 8) & 0xFF;
    buffer[buffer_size++] = w & 0xFF;
}

void emit_bytes(const uint8_t *data, int len) {
    ensure_capacity(len);
    memcpy(buffer + buffer_size, data, len);
    buffer_size += len;
}

void emit_string(const char *str) {
    int len = strlen(str);
    if (len > 255) len = 255;
    emit_byte(0x00);        /* String marker */
    emit_byte((uint8_t)len);
    emit_bytes((const uint8_t *)str, len);
}

void emit_string_n(const char *str, int len) {
    if (len > 255) len = 255;
    emit_byte(0x00);        /* String marker */
    emit_byte((uint8_t)len);
    emit_bytes((const uint8_t *)str, len);
}

void emit_add_forward_ref(int offset, const char *label, const char *proc) {
    if (forward_ref_count >= forward_ref_capacity) {
        forward_ref_capacity = forward_ref_capacity ? forward_ref_capacity * 2 : 256;
        forward_refs = realloc(forward_refs, forward_ref_capacity * sizeof(ForwardRef));
    }
    forward_refs[forward_ref_count].patch_offset = offset;
    forward_refs[forward_ref_count].label = label ? strdup(label) : NULL;
    forward_refs[forward_ref_count].proc = proc ? strdup(proc) : NULL;
    forward_ref_count++;
}

int emit_get_forward_ref_count(void) {
    return forward_ref_count;
}

ForwardRef *emit_get_forward_refs(void) {
    return forward_refs;
}

void emit_clear_forward_refs(void) {
    for (int i = 0; i < forward_ref_count; i++) {
        free((char *)forward_refs[i].label);
        free((char *)forward_refs[i].proc);
    }
    free(forward_refs);
    forward_refs = NULL;
    forward_ref_count = 0;
    forward_ref_capacity = 0;
}

void emit_patch_byte(int offset, uint8_t value) {
    if (offset >= 0 && offset < buffer_size) {
        buffer[offset] = value;
    }
}

void emit_patch_word_be(int offset, int16_t value) {
    if (offset >= 0 && offset + 1 < buffer_size) {
        buffer[offset] = (value >> 8) & 0xFF;
        buffer[offset + 1] = value & 0xFF;
    }
}

int emit_write_to_file(FILE *fp) {
    size_t written = fwrite(buffer, 1, buffer_size, fp);
    return (int)written == buffer_size ? 0 : -1;
}

const uint8_t *emit_get_buffer(int *size) {
    if (size) *size = buffer_size;
    return buffer;
}
