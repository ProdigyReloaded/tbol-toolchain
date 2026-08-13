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
#include "decode.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "opcodes.h"

/*
 * Instruction info table
 */
typedef struct {
    uint8_t opcode;
    Mnemonic mnemonic;
    const char *name;
    int operand_count;  /* -1 = variable, -2 = special handling */
    bool has_offset;    /* Has jump offset after operands */
} OpcodeInfo;

static const OpcodeInfo opcode_table[] = {
    {OP_BREAK,      MNEM_BREAK,      "BREAK",      0, false},
    {OP_CJEQ,       MNEM_CJEQ,       "CJEQ",       2, true},
    {OP_CJNE,       MNEM_CJNE,       "CJNE",       2, true},
    {OP_CJLT,       MNEM_CJLT,       "CJLT",       2, true},
    {OP_CJGT,       MNEM_CJGT,       "CJGT",       2, true},
    {OP_CJLE,       MNEM_CJLE,       "CJLE",       2, true},
    {OP_CJGE,       MNEM_CJGE,       "CJGE",       2, true},
    {OP_JUMP,       MNEM_JUMP,       "JUMP",       0, true},
    {OP_DEF_FIELD,  MNEM_DEFINE_FIELD, "DEFINE_FIELD", -2, false},
    {OP_DEF_FIELD_ST, MNEM_DEFINE_FIELD, "DEFINE_FIELD", -2, false},
    {OP_SET_ATT,    MNEM_SET_ATTRIBUTE, "SET_ATTRIBUTE", -2, false},
    {OP_MOVE,       MNEM_MOVE,       "MOVE",       2, false},
    {OP_MOVE_ABS,   MNEM_MOVE_ABS,   "MOVE_ABS",   2, false},
    {OP_SWAP,       MNEM_SWAP,       "SWAP",       2, false},
    {OP_ADD,        MNEM_ADD,        "ADD",        2, false},
    {OP_SUB,        MNEM_SUB,        "SUBTRACT",   2, false},
    {OP_MUL,        MNEM_MUL,        "MULTIPLY",   2, false},
    {OP_DIV,        MNEM_DIV,        "DIVIDE",     2, false},
    {OP_DIV_REM,    MNEM_DIV,        "DIVIDE",     3, false},
    {OP_FILL,       MNEM_FILL,       "FILL",       3, false},
    {OP_LENGTH,     MNEM_LENGTH,     "LENGTH",     2, false},
    {OP_FORMAT,     MNEM_UNKNOWN,    "FORMAT",     3, false},
    {OP_STRING,     MNEM_STRING,     "STRING",    -1, false},
    {OP_SUBSTR,     MNEM_SUBSTR,     "SUBSTR",     4, false},
    {OP_INSTR,      MNEM_INSTR,      "INSTR",      3, false},
    {OP_UPPER,      MNEM_UPPERCASE,  "UPPERCASE",  1, false},
    {OP_PUSH,       MNEM_PUSH,       "PUSH",       1, false},
    {OP_POP,        MNEM_POP,        "POP",        1, false},
    {OP_CALL,       MNEM_CALL,       "CALL",      -1, true},
    {OP_LINK,       MNEM_LINK,       "LINK",      -1, false},
    {OP_RETURN,     MNEM_RETURN,     "RETURN",     0, false},
    {OP_RETURN_RC,  MNEM_RETURN,     "RETURN",     1, false},
    {OP_TRANSFER,   MNEM_TRANSFER,   "TRANSFER",  -1, false},
    {OP_EXIT,       MNEM_EXIT,       "EXIT",       0, false},
    {OP_EXIT_RC,    MNEM_EXIT,       "EXIT",       1, false},
    {OP_GO_DEP,     MNEM_GOTO_DEPENDING_ON, "GOTO_DEPENDING_ON", -2, false},
    {OP_ERROR,      MNEM_ERROR,      "ERROR",      1, false},
    {OP_SAVE,       MNEM_SAVE,       "SAVE",      -2, false},
    {OP_SAVE_RANGE, MNEM_SAVE_FIELDS,"SAVE",       3, false},
    {OP_RESTORE,    MNEM_RESTORE,    "RESTORE",    2, false},
    {OP_RELEASE,    MNEM_RELEASE,    "RELEASE",    1, false},
    {OP_CLEAR,      MNEM_CLEAR,      "CLEAR",     -2, false},  /* Special: count byte + 1 operand */
    {OP_CLEAR_RANGE,MNEM_CLEAR_RANGE,"CLEAR",      2, false},
    {OP_NOTE,       MNEM_NOTE,       "NOTE",       2, false},
    {OP_POINT,      MNEM_POINT,      "POINT",      2, false},
    {OP_SOUND,      MNEM_SOUND,      "SOUND",      0, false},
    {OP_SOUND_ARGS, MNEM_SET_SOUND,  "SOUND",      2, false},
    {OP_SORT,       MNEM_SORT,       "SORT",       3, false},
    {OP_LOOKUP,     MNEM_LOOKUP,     "LOOKUP",     5, false},
    {OP_TRIG_FUNC,  MNEM_TRIG_FUNC,  "TRIGGER_FUNCTION", 1, false},
    /* {0x5C,          MNEM_UNKNOWN,    "SHOW_SCREEN", 1, false}, */
    /* {0x5D,          MNEM_UNKNOWN,    "UPLOAD",     2, false}, */
    /* {0x5E,          MNEM_UNKNOWN,    "DOWNLOAD",   2, false}, */
    {OP_MAKE_FORMAT,MNEM_MAKE_FORMAT,"MAKE_FORMAT",-2, false},
    {OP_NAVIGATE,   MNEM_NAVIGATE,   "NAVIGATE",   1, false},
    {OP_NAV_NEXT,   MNEM_UNKNOWN,    "NAVIGATE NEXT", 0, false},
    {OP_NAV_BACK,   MNEM_UNKNOWN,    "NAVIGATE BACK", 0, false},
    {OP_NAV_FIRST,  MNEM_UNKNOWN,    "NAVIGATE FIRST", 0, false},
    {OP_NAV_LAST,   MNEM_UNKNOWN,    "NAVIGATE LAST", 0, false},
    {OP_FETCH,      MNEM_UNKNOWN,    "FETCH",      1, false},
    {OP_FETCH_RQ,   MNEM_UNKNOWN,    "FETCH",      2, false},
    {OP_SET_FUNC,   MNEM_SET_FUNCTION,"SET_FUNCTION", 2, false},
    {OP_SET_FUNC_PGM, MNEM_SET_FUNCTION,"SET_FUNCTION", 3, false},
    {OP_SET_FUNC2,  MNEM_SET_FUNCTION,"SET_FUNCTION", 4, false},
    {OP_OPEN_WINDOW, MNEM_UNKNOWN,   "OPEN_WINDOW", 1, false},
    {OP_OPEN_ERR_WIN, MNEM_UNKNOWN,  "OPEN_ERROR_WINDOW", 1, false},
    {OP_CLOSE_WINDOW, MNEM_UNKNOWN,  "CLOSE_WINDOW", 0, false},
    {OP_CLOSE_OPEN_WIN, MNEM_UNKNOWN,"CLOSE_WINDOW", 1, false},
    {OP_KILL,       MNEM_UNKNOWN,    "KILL",       1, false},
    {OP_PURGE,      MNEM_UNKNOWN,    "PURGE_CACHE", 0, false},
    {OP_AND,        MNEM_UNKNOWN,    "AND",        2, false},
    {OP_OR,         MNEM_UNKNOWN,    "OR",         2, false},
    {OP_XOR,        MNEM_UNKNOWN,    "XOR",        2, false},
    {OP_TEST,       MNEM_UNKNOWN,    "TEST",       2, false},
    {OP_EDIT,       MNEM_UNKNOWN,    "EDIT",      -1, false},
    /* Patent-only opcodes - commented out (see defines above) */
    /* {0x3C,          MNEM_UNKNOWN,    "SYNC_SAVE",  0, false}, */
    /* {0x3D,          MNEM_UNKNOWN,    "SYNC_RELEASE", 0, false}, */
    /* {0x3E,          MNEM_UNKNOWN,    "TIMER_ON",   1, false}, */
    /* {0x61,          MNEM_UNKNOWN,    "TIMER_OFF",  1, false}, */
    {OP_START,      MNEM_UNKNOWN,    "START",      1, false},
    {OP_STOP,       MNEM_UNKNOWN,    "STOP",       1, false},
    {OP_SET_KEY,    MNEM_UNKNOWN,    "SET_KEY",    3, false},
    {OP_SET_KEY_PGM,MNEM_UNKNOWN,    "SET_KEY",    5, false},
    /* {0x59,          MNEM_UNKNOWN,    "SET_BACKGROUND", 1, false}, */
    /* {0x5B,          MNEM_UNKNOWN,    "FILE_SCREEN", 1, false}, */
    /* {0x5F,          MNEM_UNKNOWN,    "ACCESS",     2, false}, */
    {OP_MOVE_BLOCK, MNEM_UNKNOWN,    "MOVE_BLOCK", -2, false},
    {OP_SET_CURSOR, MNEM_UNKNOWN,    "SET_CURSOR", 1, false},
    {OP_OPEN_ERR_WIN2, MNEM_UNKNOWN, "OPEN_ERROR_WINDOW", 1, false},
    {OP_OPEN_ERR_WIN3, MNEM_UNKNOWN, "OPEN_ERROR_WINDOW", 2, false},
    {OP_DELETE,     MNEM_UNKNOWN,    "DELETE",     1, false},
    {OP_OPEN,       MNEM_UNKNOWN,    "OPEN",       2, false},
    {OP_CLOSE,      MNEM_UNKNOWN,    "CLOSE",      1, false},
    {OP_CLOSE_ALL,  MNEM_UNKNOWN,    "CLOSE_ALL",  0, false},
    {OP_READ,       MNEM_UNKNOWN,    "READ",       2, false},
    {OP_READ3,      MNEM_UNKNOWN,    "READ",       3, false},
    {OP_WRITE,      MNEM_UNKNOWN,    "WRITE",      2, false},
    {OP_WRITE3,     MNEM_UNKNOWN,    "WRITE",      3, false},
    {OP_CONNECT,    MNEM_UNKNOWN,    "CONNECT",    1, false},
    {OP_DISCONNECT, MNEM_UNKNOWN,    "DISCONNECT", 0, false},
    {OP_SEND,       MNEM_SEND,       "SEND",      -2, false},  /* Special: operands + 3 trailing bytes */
    {OP_SEND_ID,    MNEM_SEND,       "SEND",      -2, false},  /* Special: operands + 3 trailing bytes */
    {OP_RECEIVE,    MNEM_UNKNOWN,    "RECEIVE",    2, false},
    {OP_CANCEL,     MNEM_UNKNOWN,    "CANCEL",     0, false},
    {OP_REFRESH,    MNEM_UNKNOWN,    "REFRESH",    0, false},
    {OP_ERASE,      MNEM_UNKNOWN,    "ERASE",      1, false},
    {OP_WAIT,       MNEM_UNKNOWN,    "WAIT",       0, false},
    {0, MNEM_UNKNOWN, NULL, 0, false}  /* Sentinel */
};

static const OpcodeInfo *lookup_opcode(uint8_t opcode) {
    for (int i = 0; opcode_table[i].name != NULL; i++) {
        if (opcode_table[i].opcode == opcode) {
            return &opcode_table[i];
        }
    }
    return NULL;
}

const char *mnemonic_str(Mnemonic m) {
    for (int i = 0; opcode_table[i].name != NULL; i++) {
        if (opcode_table[i].mnemonic == m) {
            return opcode_table[i].name;
        }
    }
    return "UNKNOWN";
}

/*
 * Decode a single operand byte value to an Operand
 */
static Operand *decode_operand_value(int value, bool is_extended) {
    if (is_extended) {
        /* Extended 16-bit value */
        if (value >= 0x0100 && value <= 0x013F) {
            /* RDA 158-221 */
            return operand_new(OP_RDA, 158 + (value - 0x0100));
        }
        if (value >= 0x0140 && value <= 0x01FF) {
            /* PEV &65-&256 */
            return operand_new(OP_PEV, 65 + (value - 0x0140));
        }
        if (value >= 0x0200 && value <= 0x7EFF) {
            /* GEV #1-#32000 */
            return operand_new(OP_GEV, value - 0x0200 + 1);
        }
        /* Unknown extended value */
        return operand_new(OP_LITERAL_NUM, value);
    }

    /* Simple 1-byte value */
    if (value >= 0x01 && value <= 0x08) {
        /* Small numeric literals 1-8 */
        return operand_new(OP_LITERAL_NUM, value);
    }
    if (value >= 0x09 && value <= 0x10) {
        /* I1-I8 */
        return operand_new(OP_REG_I, value - 0x08);
    }
    if (value >= 0x11 && value <= 0x18) {
        /* D1-D8 */
        return operand_new(OP_REG_D, value - 0x10);
    }
    if (value >= 0x19 && value <= 0x21) {
        /* P0-P8 */
        return operand_new(OP_REG_P, value - 0x19);
    }
    if (value >= 0x22 && value <= 0xBF) {
        /* RDA 0-157 */
        return operand_new(OP_RDA, value - 0x22);
    }
    if (value >= 0xC0 && value <= 0xFF) {
        /* PEV &1-&64 */
        return operand_new(OP_PEV, value - 0xC0 + 1);
    }

    /* Unknown */
    return operand_new(OP_LITERAL_NUM, value);
}

/*
 * Read one operand from the code stream
 */
static Operand *read_operand(const uint8_t *code, int *pos, int size, bool is_complex_op) {
    if (*pos >= size) return NULL;

    uint8_t b = code[(*pos)++];

    /* String literal: starts with 0x00 */
    if (b == 0x00) {
        if (*pos >= size) return NULL;
        uint8_t len = code[(*pos)++];
        if (*pos + len > size) return NULL;
        Operand *op = operand_new_str((const char *)&code[*pos], len);
        *pos += len;
        return op;
    }

    /* Extended operand (2-byte): check if complex mode indicates this */
    if (is_complex_op) {
        /* Read second byte */
        if (*pos >= size) return operand_new(OP_LITERAL_NUM, b);
        uint8_t b2 = code[(*pos)++];
        int value = (b << 8) | b2;

        /* Check for indexed operand (high bit set on base) */
        if (value & 0x8000) {
            int base_value = value & 0x7FFF;
            if (*pos >= size) return decode_operand_value(base_value, true);
            uint8_t idx_byte = code[(*pos)++];
            /* For indexed operands, base uses simple encoding if low byte only,
             * or extended encoding if high byte is non-zero */
            bool base_is_extended = (base_value > 0xFF);
            Operand *base = decode_operand_value(base_is_extended ? base_value : (base_value & 0xFF),
                                                  base_is_extended);
            /* Index uses linear encoding: 34 + slot for RDA, NOT standard operand encoding */
            /* This allows slots 0-221 in a single byte */
            Operand *idx;
            if (idx_byte >= 34) {
                /* RDA slot = idx_byte - 34 */
                idx = operand_new(OP_RDA, idx_byte - 34);
            } else if (idx_byte >= 0x09 && idx_byte <= 0x10) {
                /* I1-I8 */
                idx = operand_new(OP_REG_I, idx_byte - 0x08);
            } else if (idx_byte >= 0x11 && idx_byte <= 0x18) {
                /* D1-D8 */
                idx = operand_new(OP_REG_D, idx_byte - 0x10);
            } else if (idx_byte >= 0x19 && idx_byte <= 0x21) {
                /* P0-P8 */
                idx = operand_new(OP_REG_P, idx_byte - 0x19);
            } else {
                /* Small literals 1-8 */
                idx = operand_new(OP_LITERAL_NUM, idx_byte);
            }
            return operand_new_indexed(base, idx);
        }

        return decode_operand_value(value, true);
    }

    /* Simple 1-byte operand */
    return decode_operand_value(b, false);
}

/*
 * Read a 16-bit big-endian signed offset
 */
static int16_t read_offset(const uint8_t *code, int *pos, int size) {
    if (*pos + 2 > size) return 0;
    int16_t offset = (int16_t)((code[*pos] << 8) | code[*pos + 1]);
    *pos += 2;
    return offset;
}

/*
 * Special-case opcode decoders
 * Each reads operands and populates the instruction for one opcode family.
 */

static void decode_clear(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* CLEAR: count byte then 1 operand (start slot) */
    /* Count indicates how many consecutive slots to clear */
    (void)mode_byte;
    if (*pos >= size) return;
    instr->var_count = code[(*pos)++];
    /* Fall through to generic operand reading with op_count = 1 */
}

static void decode_goto_dep(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* GOTO_DEPENDING_ON: 1 operand, then count, then count offsets
     * Each offset is relative to the position AFTER that offset in bytecode */
    bool is_ext = instr->is_complex && (mode_byte & 0x80);
    Operand *op = read_operand(code, pos, size, is_ext);
    if (op) instruction_add_operand(instr, op);

    if (*pos >= size) return;
    int label_count = code[(*pos)++];
    instr->var_count = label_count;

    /* Read label offsets and convert to absolute targets.
     * Forward offsets are relative to the position after each offset.
     * Backward offsets are relative to the end of the entire instruction
     * (a quirk of the original TBOL compiler). */
    int offsets_start = *pos;
    int16_t raw_offsets[256];
    for (int i = 0; i < label_count && i < 256 && *pos + 2 <= size; i++)
        raw_offsets[i] = read_offset(code, pos, size);

    uint16_t instr_end = (uint16_t)*pos;

    /* Resolve: rewind pos to re-walk for per-offset bases */
    int p = offsets_start;
    for (int i = 0; i < label_count && i < 256; i++) {
        p += 2; /* advance past this offset */
        uint16_t base = (raw_offsets[i] < 0) ? instr_end : (uint16_t)p;
        uint16_t target = base + raw_offsets[i];
        instruction_add_operand(instr, operand_new(OP_LITERAL_NUM, target));
    }
}

static void decode_make_format(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* MAKE_FORMAT: count byte, then (count*3 + 1) operands
     * Format: dest, then for each spec: target, fix_len, embed_len
     * Count byte gives number of specs */
    if (*pos >= size) return;
    int spec_count = code[(*pos)++];
    instr->var_count = spec_count;
    int total_operands = spec_count * 3 + 1;

    /* Read additional mode bytes if needed (>8 operands). Sized for the
     * max: spec_count is a uint8_t, so total_operands <= 766 -> up to 95
     * extra mode bytes plus mode_bytes[0]. (Was [32], too small past ~85
     * specs.) */
    uint8_t mode_bytes[96] = {0};
    mode_bytes[0] = mode_byte;
    if (instr->is_complex && total_operands > 8) {
        int extra_mode_bytes = (total_operands - 8 + 7) / 8;
        for (int m = 0; m < extra_mode_bytes && *pos < size; m++) {
            mode_bytes[m + 1] = code[(*pos)++];
        }
    }

    /* Read all operands */
    for (int i = 0; i < total_operands && *pos < size; i++) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        bool is_ext = instr->is_complex && (mode_bytes[byte_idx] & (1 << bit_idx));
        Operand *op = read_operand(code, pos, size, is_ext);
        if (op) instruction_add_operand(instr, op);
    }
}

static void decode_define_field(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* Bytecode layout: name, col, width, height, object_id [, state], lo_hi_byte
     * Source syntax:   DEFINE_FIELD name, row, col, width, height, object_id [, state];
     * Row is omitted from bytecode.  lo_hi_byte (always 0x01) is not a source operand.
     * The 7-operand form (OP_DEF_FIELD_ST, 0x09) adds a state operand before lo_hi.
     */
    bool has_state = (instr->opcode == OP_DEF_FIELD_ST);

    /* Mode byte bits: b7=name, b6=col, b5=width, b4=height */
    bool is_ext = instr->is_complex && (mode_byte & 0x80);
    Operand *name = read_operand(code, pos, size, is_ext);

    Operand *col = NULL, *width = NULL, *height = NULL;
    is_ext = instr->is_complex && (mode_byte & 0x40);
    col = read_operand(code, pos, size, is_ext);
    is_ext = instr->is_complex && (mode_byte & 0x20);
    width = read_operand(code, pos, size, is_ext);
    is_ext = instr->is_complex && (mode_byte & 0x10);
    height = read_operand(code, pos, size, is_ext);

    /* object_id: always extended (string literal) */
    Operand *object_id = read_operand(code, pos, size, false);

    /* state (7-operand form only) */
    Operand *state = NULL;
    if (has_state && *pos < size) {
        state = read_operand(code, pos, size, false);
    }

    /* Consume lo_hi flag byte */
    if (*pos < size) (*pos)++;

    /* Emit operands in source order: name, col, width, height, object_id [, state] */
    if (name)      instruction_add_operand(instr, name);
    if (col)       instruction_add_operand(instr, col);
    if (width)     instruction_add_operand(instr, width);
    if (height)    instruction_add_operand(instr, height);
    if (object_id) instruction_add_operand(instr, object_id);
    if (state)     instruction_add_operand(instr, state);
}

static void decode_set_attribute(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* SET_ATTRIBUTE: field operand + 4 attribute bytes + optional form string */
    bool is_ext = instr->is_complex && (mode_byte & 0x80);
    Operand *field = read_operand(code, pos, size, is_ext);
    if (field) instruction_add_operand(instr, field);
    /* 4 attribute bytes */
    uint8_t attr_bytes[4] = {0};
    for (int i = 0; i < 4 && *pos < size; i++) {
        attr_bytes[i] = code[(*pos)++];
        Operand *ab = operand_new(OP_LITERAL_NUM, attr_bytes[i]);
        instruction_add_operand(instr, ab);
    }
    /* Check if byte 1 has FORM flag (0x20) */
    if ((attr_bytes[1] & 0x20) && *pos < size) {
        /* Read form string: length byte + string */
        uint8_t form_len = code[(*pos)++];
        if (*pos + form_len <= size) {
            char *form_str = malloc(form_len + 1);
            if (form_str) {
                memcpy(form_str, &code[*pos], form_len);
                form_str[form_len] = '\0';
                *pos += form_len;
                Operand *form_op = operand_new(OP_LITERAL_STR, 0);
                form_op->str_value = form_str;
                instruction_add_operand(instr, form_op);
            }
        }
    }
}

static void decode_save(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* SAVE: count byte then 2 operands (name, start slot) */
    /* Count indicates how many consecutive slots to save */
    (void)mode_byte;
    if (*pos >= size) return;
    instr->var_count = code[(*pos)++];
    /* Fall through to generic operand reading with op_count = 2 */
}

static void decode_move_block(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* MOVE_BLOCK: count byte then 2 operands (src, dst) */
    (void)mode_byte;
    if (*pos >= size) return;
    instr->var_count = code[(*pos)++];
    /* Fall through to generic operand reading with op_count = 2 */
}

static void decode_send(Instruction *instr, const uint8_t *code, int *pos, int size, uint8_t mode_byte) {
    /* SEND: 1 operand + 3 trailing bytes (timeout + flags)
     * SEND_ID: 2 operands + 3 trailing bytes (timeout + flags) */
    int send_op_count = (instr->opcode == OP_SEND) ? 1 : 2;

    /* Read operands */
    for (int i = 0; i < send_op_count && *pos < size; i++) {
        bool is_ext = instr->is_complex && (mode_byte & (0x80 >> i));
        Operand *op = read_operand(code, pos, size, is_ext);
        if (op) instruction_add_operand(instr, op);
    }

    /* Read 3 trailing bytes: 2-byte timeout (little-endian) + 1-byte flags */
    if (*pos + 3 <= size) {
        uint8_t timeout_lo = code[(*pos)++];
        uint8_t timeout_hi = code[(*pos)++];
        instr->send_timeout = (int16_t)((timeout_hi << 8) | timeout_lo);
        instr->send_flags = code[(*pos)++];
    }
}

/*
 * Decode a single instruction
 */
static Instruction *decode_instruction(const uint8_t *code, int *pos, int size) {
    if (*pos >= size) return NULL;

    int start_pos = *pos;
    Instruction *instr = instruction_new(start_pos);

    /* Read opcode */
    instr->raw_opcode = code[(*pos)++];
    instr->is_complex = (instr->raw_opcode & OP_COMPLEX) != 0;
    instr->opcode = instr->raw_opcode & 0x7F;

    /* Lookup opcode info */
    const OpcodeInfo *info = lookup_opcode(instr->opcode);
    if (!info) {
        /* Unknown opcode */
        instr->mnemonic = MNEM_UNKNOWN;
        instr->mnem_str = "???";
        instr->length = *pos - start_pos;
        return instr;
    }

    instr->mnemonic = info->mnemonic;
    instr->mnem_str = info->name;
    instr->has_jump = info->has_offset;

    /* Read mode byte if complex */
    uint8_t mode_byte = 0;
    if (instr->is_complex && *pos < size) {
        mode_byte = code[(*pos)++];
        instr->mode_byte = mode_byte;
    }

    /* Determine operand count and decode operands */
    int op_count = info->operand_count;

    if (op_count == -1) {
        /* Variable args: count byte first */
        if (*pos >= size) goto done;
        op_count = code[(*pos)++];
        instr->var_count = op_count;
    } else if (op_count == -2) {
        /* Special handling per opcode */
        if (instr->opcode == OP_CLEAR) {
            decode_clear(instr, code, pos, size, mode_byte);
            op_count = 1;  /* Just the start operand */
        } else if (instr->opcode == OP_GO_DEP) {
            decode_goto_dep(instr, code, pos, size, mode_byte);
            goto done;
        } else if (instr->opcode == OP_MAKE_FORMAT) {
            decode_make_format(instr, code, pos, size, mode_byte);
            goto done;
        } else if (instr->opcode == OP_DEF_FIELD || instr->opcode == OP_DEF_FIELD_ST) {
            decode_define_field(instr, code, pos, size, mode_byte);
            goto done;
        } else if (instr->opcode == OP_SET_ATT) {
            decode_set_attribute(instr, code, pos, size, mode_byte);
            goto done;
        } else if (instr->opcode == OP_SAVE) {
            decode_save(instr, code, pos, size, mode_byte);
            op_count = 2;  /* Name and start slot operands */
        } else if (instr->opcode == OP_MOVE_BLOCK) {
            decode_move_block(instr, code, pos, size, mode_byte);
            op_count = 2;
        } else if (instr->opcode == OP_SEND || instr->opcode == OP_SEND_ID) {
            decode_send(instr, code, pos, size, mode_byte);
            goto done;
        } else {
            /* Default: no operands */
            op_count = 0;
        }
    }

    /* Read additional mode bytes if needed for variable-arg instructions (>8 operands) */
    uint8_t mode_bytes[32] = {0};
    mode_bytes[0] = mode_byte;
    if (instr->is_complex && op_count > 8) {
        int extra_mode_bytes = (op_count - 8 + 7) / 8;
        for (int m = 0; m < extra_mode_bytes && *pos < size; m++) {
            mode_bytes[m + 1] = code[(*pos)++];
        }
    }

    /* Read operands */
    for (int i = 0; i < op_count && *pos < size; i++) {
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        bool is_ext = instr->is_complex && (mode_bytes[byte_idx] & (1 << bit_idx));
        Operand *op = read_operand(code, pos, size, is_ext);
        if (op) instruction_add_operand(instr, op);
    }

    /* Read jump offset if present */
    if (info->has_offset && *pos + 2 <= size) {
        instr->jump_offset = read_offset(code, pos, size);
        instr->jump_target = *pos + instr->jump_offset;  /* Relative to end of instruction */
    }

done:
    instr->length = *pos - start_pos;
    return instr;
}

/*
 * Decode all instructions in a program
 */
int decode_program(Program *prog) {
    if (!prog || !prog->code) return -1;

    Instruction *head = NULL;
    Instruction *tail = NULL;
    int pos = 0;

    while (pos < prog->code_size) {
        Instruction *instr = decode_instruction(prog->code, &pos, prog->code_size);
        if (!instr) break;

        if (!head) {
            head = tail = instr;
        } else {
            tail->next = instr;
            tail = instr;
        }
        prog->instr_count++;
    }

    prog->instructions = head;
    return 0;
}
