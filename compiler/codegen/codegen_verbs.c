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
 * TBOL Compiler - Verb Code Generation
 */

#include "codegen_internal.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*
 * SET_FUNCTION symbolic name lookup tables
 */
static const struct {
    const char *name;
    int code;
} function_names[] = {
    {"NO_EVENT", 0}, {"ADD_TEXT", 1}, {"ADD_TEXT_AND_FIELD_END", 2},
    {"BACKSPACE", 3}, {"DELETE_TEXT", 4}, {"TOGGLE_INSERT", 5},
    {"HELP", 6}, {"INSERT_TEXT", 7}, {"FIELD_HELP", 8}, {"PAGE_HELP", 9},
    {"NAVIGATION", 10}, {"INTERFIELD_CURSOR", 11}, {"NEXT", 12}, {"BACK", 13},
    {"PATH", 14}, {"JUMP", 15}, {"ACTION", 16}, {"FIELD_END", 17},
    {"ELEMENT_END", 18}, {"PAGE_END", 19}, {"BYE", 20}, {"LOGON", 21},
    {"VIEWPATH", 22}, {"KEYWORD", 23}, {"GUIDE", 24}, {"RECALL", 25},
    {"UNDO", 26}, {"WHERE", 27}, {"SRNPRT", 28}, {"SMFILE", 29},
    {"SCAN", 30}, {"TOOLS", 31}, {"DIRECTORY", 32}, {"INDEX", 33},
    {"FIND", 34}, {"LEAVE", 35}, {"ZIP", 36}, {"NEW_LINE", 37},
    {"SCROLL_UP", 38}, {"SCROLL_DOWN", 39}, {"SET_CLOSE_WINDOW", 40},
    {"SET_OPEN_WINDOW", 41}, {"CURSOR", 42}, {"LOGOFF", 43}, {"LOOK", 44},
    {"TRAVEL", 45}, {"FIRST_PAGE", 46}, {"LAST_PAGE", 47},
    {"PREVIOUS_MENU", 48}, {"LOGICAL_FUNCTIONS", 49},
    {NULL, -1}
};

static const struct {
    const char *name;
    int code;
} action_names[] = {
    {"NORMAL", 0}, {"DISABLE", 16}, {"FILTER", 32}, {"FILTER_ON", 64},
    {NULL, -1}
};

/*
 * Look up a SET_FUNCTION function name and return its code, or -1 if not found
 */
static int lookup_function_name(const char *name) {
    for (int i = 0; function_names[i].name; i++) {
        if (strcasecmp(name, function_names[i].name) == 0) {
            return function_names[i].code;
        }
    }
    return -1;
}

/*
 * Look up a SET_FUNCTION action name and return its code, or -1 if not found
 */
static int lookup_action_name(const char *name) {
    for (int i = 0; action_names[i].name; i++) {
        if (strcasecmp(name, action_names[i].name) == 0) {
            return action_names[i].code;
        }
    }
    return -1;
}

/*
 * Generate SET_FUNCTION instruction with symbolic name support
 */
static void gen_set_function(AstNode *node) {
    int count = node->child_count;
    if (count < 2) return;

    AstNode *operands[4];
    for (int i = 0; i < count && i < 4; i++) {
        operands[i] = node->children[i];
    }

    /* Check if first operand is a symbolic function name */
    AstNode *func_op = operands[0];
    int func_code = -1;
    if (func_op->kind == AST_IDENT) {
        func_code = lookup_function_name(func_op->data.ident.name);
    }

    /* Check if second operand is a symbolic action name */
    AstNode *action_op = operands[1];
    int action_code = -1;
    if (action_op->kind == AST_IDENT) {
        action_code = lookup_action_name(action_op->data.ident.name);
    }

    /* Determine opcode based on operand count */
    uint8_t opcode;
    if (count == 2) {
        opcode = OP_SET_FUNC;
    } else if (count == 3) {
        opcode = OP_SET_FUNC_PGM;
    } else {
        opcode = OP_SET_FUNC2;
    }

    /* Check complex mode - but skip converted operands (they become simple strings) */
    AstNode *check_ops[4];
    int check_count = 0;
    for (int i = 0; i < count; i++) {
        if (i == 0 && func_code >= 0) continue;  /* Will emit as simple string */
        if (i == 1 && action_code >= 0) continue;
        check_ops[check_count++] = operands[i];
    }

    bool complex = (check_count > 0) ? check_complex_mode(check_ops, check_count) : false;

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        /* Compute mode byte for all operands */
        uint8_t mode = 0;
        for (int i = 0; i < count; i++) {
            if ((i == 0 && func_code >= 0) || (i == 1 && action_code >= 0)) {
                /* Converted to simple string - no extended encoding */
                continue;
            }
            if (needs_extended_encoding(operands[i])) {
                mode |= (1 << (7 - i));
            }
        }
        emit_byte(mode);
    } else {
        emit_byte(opcode);
    }

    /* Emit operands */
    for (int i = 0; i < count; i++) {
        if (i == 0 && func_code >= 0) {
            /* Emit function code as string */
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", func_code);
            emit_string(buf);
        } else if (i == 1 && action_code >= 0) {
            /* Emit action code as string */
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", action_code);
            emit_string(buf);
        } else {
            emit_operand(operands[i], needs_extended_encoding(operands[i]));
        }
    }
}

/*
 * Parse SET_ATTRIBUTE attribute string and emit encoding
 * Format: "CONTEXT TYPE COLOR(fg,bg)" or "CONTEXT COLOR(fg,bg)" etc.
 * Context: ACTION=0x80, DISPLAY=0x40, INPUT=0x20, PP_NOT_FIRED=0x08,
 *          ALPHANUMERIC=0x00
 * Type: ALPHABETIC=0x80, NUMERIC=0x40, FORM=0x20, PASSWORD=0x10
 *
 * PP_NOT_FIRED marks an INPUT-like field whose change does not fire the
 * post-processor chain (the inverse of ACTION). Empirically TBOL.EXE
 * lets PP_NOT_FIRED override an explicit INPUT/DISPLAY in the same
 * string, encoding only 0x08 - so we match it before the others.
 */
static void gen_set_attribute(AstNode *node) {
    if (node->child_count < 2) return;

    AstNode *field = node->children[0];
    AstNode *attr = node->children[1];

    /* Emit opcode (potentially with complex mode) */
    AstNode *operands[1] = {field};
    bool complex = check_complex_mode(operands, 1);
    if (complex) {
        emit_byte(OP_SET_ATT | OP_COMPLEX);
        emit_byte(compute_mode_byte(operands, 1));
    } else {
        emit_byte(OP_SET_ATT);
    }

    /* Emit field operand */
    emit_operand(field, needs_extended_encoding(field));

    /* Parse attribute string */
    uint8_t context_byte = 0x00;  /* Default: ALPHANUMERIC */
    uint8_t type_byte = 0x00;     /* Default: none */
    uint8_t fg_color = 0x80;      /* 0x80 = no color */
    uint8_t bg_color = 0x80;

    /* Get the attribute string */
    const char *attr_str = NULL;
    if (attr->kind == AST_LITERAL_STR) {
        attr_str = attr->data.str_lit.value;
    }

    if (attr_str) {
        /* Parse context. PP_NOT_FIRED is checked first because TBOL.EXE
         * lets it override INPUT/DISPLAY in the same string. */
        if (strstr(attr_str, "PP_NOT_FIRED")) {
            context_byte = 0x08;
        } else if (strstr(attr_str, "ACTION")) {
            context_byte = 0x80;
        } else if (strstr(attr_str, "DISPLAY")) {
            context_byte = 0x40;
        } else if (strstr(attr_str, "INPUT")) {
            context_byte = 0x20;
        }

        /* Parse type (check for word boundaries to avoid "ALPHANUMERIC" matching "NUMERIC") */
        if (strstr(attr_str, "ALPHABETIC")) {
            type_byte = 0x80;
        } else if (strstr(attr_str, "PASSWORD")) {
            type_byte = 0x10;
        } else {
            /* Check for standalone NUMERIC (with space before or at start) */
            const char *num = strstr(attr_str, " NUMERIC");
            if (!num && strncmp(attr_str, "NUMERIC", 7) == 0) {
                num = attr_str;  /* NUMERIC at start of string */
            }
            if (num) {
                type_byte = 0x40;
            }
        }
        /* Note: FORM is checked separately below since we need to extract the content */

        /* Parse COLOR(fg,bg) */
        const char *color = strstr(attr_str, "COLOR(");
        if (color) {
            int fg, bg;
            if (sscanf(color, "COLOR(%d,%d)", &fg, &bg) == 2) {
                fg_color = (uint8_t)fg;
                bg_color = (uint8_t)bg;
            }
        }

        /* Parse FORM(content) */
        const char *form = strstr(attr_str, "FORM(");
        if (form) {
            type_byte |= 0x20;
        }
    }

    emit_byte(context_byte);
    emit_byte(type_byte);
    emit_byte(fg_color);
    emit_byte(bg_color);

    /* If FORM type, emit form string with length prefix */
    if (attr_str && (type_byte & 0x20)) {
        const char *form = strstr(attr_str, "FORM(");
        if (form) {
            form += 5;  /* Skip "FORM(" */
            /* Find matching ')' accounting for nested parens */
            int depth = 1;
            const char *end = form;
            while (*end && depth > 0) {
                if (*end == '(') depth++;
                else if (*end == ')') depth--;
                if (depth > 0) end++;
            }
            if (depth == 0) {
                size_t len = end - form;
                emit_byte((uint8_t)len);
                /* Emit FORM content verbatim -- the TBOL form grammar
                 * is case-sensitive (e.g. 'n' = optional digit vs 'N'
                 * = required digit in a date mask). Uppercasing here
                 * would silently turn 'nN/nN/NN' into 'NN/NN/NN',
                 * which produced a 22-byte MISMATCH against QAPEINIT.
                 */
                for (size_t i = 0; i < len; i++) {
                    emit_byte((uint8_t)form[i]);
                }
            }
        }
    }
}

/*
 * Generate MAKE_FORMAT instruction
 * Syntax: MAKE_FORMAT fmt, field1:width1, field2:width2, ...
 * Encoding: opcode, [mode], count, [extra_modes...], fmt, (field, fix_len, imbed_len)...
 *
 * Total operands = field_count * 3 + 1 (the +1 is the map destination)
 * If >8 operands and complex, additional mode bytes are emitted after count.
 */
static void gen_make_format(AstNode *node) {
    if (node->child_count < 2) return;

    /* First operand is the format map destination */
    AstNode *fmt = node->children[0];

    /* Count field definitions (remaining operands are field:width pairs stored as AST_FORMAT_SPEC) */
    int field_count = 0;
    for (int i = 1; i < node->child_count; i++) {
        if (node->children[i] && node->children[i]->kind == AST_FORMAT_SPEC) {
            field_count++;
        }
    }

    /* Total operands: 1 (map dest) + field_count * 3 (field, fix_len, imbed_len) */
    int total_operands = field_count * 3 + 1;

    /* Check if any operand needs extended encoding */
    bool complex = needs_extended_encoding(fmt);
    for (int i = 1; i < node->child_count && !complex; i++) {
        AstNode *spec = node->children[i];
        if (spec && spec->kind == AST_FORMAT_SPEC && spec->child_count > 0) {
            if (needs_extended_encoding(spec->children[0])) {
                complex = true;
            }
        }
    }

    /* Calculate additional mode bytes needed (for operands beyond first 8) */
    int extra_mode_bytes = 0;
    if (complex && total_operands > 8) {
        int extra_operands = total_operands - 8;
        extra_mode_bytes = (extra_operands + 7) / 8;  /* ceil division */
    }

    /* Emit opcode */
    if (complex) {
        emit_byte(OP_MAKE_FORMAT | OP_COMPLEX);
        /* First mode byte - covers operands 0-7 */
        uint8_t mode = 0;
        if (needs_extended_encoding(fmt)) {
            mode |= (1 << 7);  /* bit 7 = operand 0 (fmt) */
        }
        /* Operands 1-7 are field dests (every 3rd) and literals (always simple) */
        int bit = 6;
        for (int i = 1; i < node->child_count && bit >= 0; i++) {
            AstNode *spec = node->children[i];
            if (spec && spec->kind == AST_FORMAT_SPEC && spec->child_count > 0) {
                if (needs_extended_encoding(spec->children[0])) {
                    mode |= (1 << bit);
                }
                bit -= 3;  /* Skip 3 bits per field (field, fix_len, imbed_len) */
            }
        }
        emit_byte(mode);
    } else {
        emit_byte(OP_MAKE_FORMAT);
    }

    /* Emit field count */
    emit_byte((uint8_t)field_count);

    /* Emit additional mode bytes after count.
     * Each bit corresponds to an operand beyond the first 8.
     * Only field operands (every 3rd starting from operand 1) can need extended encoding.
     */
    if (extra_mode_bytes > 0) {
        /* One bit per operand beyond the first 8; the count byte is a
         * uint8_t, so field_count <= 255 -> total_operands <= 766 -> at
         * most 95 extra mode bytes. (Was [8], which overflowed at >= 24
         * fields when the destination forced complex encoding.) */
        uint8_t extra_modes[96] = {0};
        /* Build list of which operands need extended encoding.
         * Operand layout: 0=fmt, then triplets (field, fix_len, imbed_len).
         * Only the field operand in each triplet can be extended. */
        int field_idx = 0;
        for (int i = 1; i < node->child_count; i++) {
            AstNode *spec = node->children[i];
            if (spec && spec->kind == AST_FORMAT_SPEC && spec->child_count > 0) {
                /* This field's operand index = 1 + field_idx * 3 */
                int op_idx = 1 + field_idx * 3;
                if (op_idx >= 8 && needs_extended_encoding(spec->children[0])) {
                    int extra_bit = op_idx - 8;
                    int byte_num = extra_bit / 8;
                    int bit_pos = 7 - (extra_bit % 8);
                    if (byte_num < extra_mode_bytes) {
                        extra_modes[byte_num] |= (1 << bit_pos);
                    }
                }
                field_idx++;
            }
        }
        for (int m = 0; m < extra_mode_bytes; m++) {
            emit_byte(extra_modes[m]);
        }
    }

    /* Emit format map operand */
    emit_operand(fmt, needs_extended_encoding(fmt));

    /* Emit each field definition: field, fix_len, imbed_len
     *
     * Format spec encoding:
     *   target:fix_len        -> fix_len as string, empty imbed_len
     *   target::imbed_len     -> empty fix_len, imbed_len as string
     *   target:fix_len:embed  -> both as strings
     */
    for (int i = 1; i < node->child_count; i++) {
        AstNode *spec = node->children[i];
        if (spec && spec->kind == AST_FORMAT_SPEC) {
            /* Field operand */
            AstNode *field = spec->children[0];
            emit_operand(field, needs_extended_encoding(field));

            /* Fixed length as string literal (empty if -1/not specified) */
            if (spec->data.format_spec.fix_len >= 0) {
                char fix_str[16];
                snprintf(fix_str, sizeof(fix_str), "%d", spec->data.format_spec.fix_len);
                emit_string(fix_str);
            } else {
                emit_byte(0x00);  /* String marker */
                emit_byte(0x00);  /* Length 0 = empty */
            }

            /* Embedded length as string literal (empty if -1/not specified) */
            if (spec->data.format_spec.imbed_len >= 0) {
                char embed_str[16];
                snprintf(embed_str, sizeof(embed_str), "%d", spec->data.format_spec.imbed_len);
                emit_string(embed_str);
            } else {
                emit_byte(0x00);  /* String marker */
                emit_byte(0x00);  /* Length 0 = empty */
            }
        }
    }
}

/*
 * Generate DEFINE_FIELD instruction
 * Syntax: DEFINE_FIELD name, row, col, width, height, object_id [, state]
 * Bytecode: opcode, name, col, width, height, object_id_value, type_byte
 * Note: row is omitted from bytecode! Object_id is emitted as string literal.
 */
static void gen_define_field(AstNode *node) {
    int count = node->child_count;
    if (count < 6) return;

    AstNode **operands = node->children;
    /* 7-operand form uses a different opcode (0x09 vs 0x08) */
    uint8_t opcode = (count >= 7) ? OP_DEF_FIELD_ST : OP_DEF_FIELD;

    /* Build the operand list for bytecode: name, col, width, height (skip row!) */
    AstNode *bc_operands[4] = {
        operands[0],  /* name */
        operands[2],  /* col (skip row at index 1) */
        operands[3],  /* width */
        operands[4]   /* height */
    };

    /* Check if any operand needs extended encoding */
    bool complex = check_complex_mode(bc_operands, 4);

    if (complex) {
        emit_byte(opcode | OP_COMPLEX);
        emit_byte(compute_mode_byte(bc_operands, 4));
    } else {
        emit_byte(opcode);
    }

    /* Emit 4 operands: name, col, width, height */
    for (int i = 0; i < 4; i++) {
        emit_operand(bc_operands[i], needs_extended_encoding(bc_operands[i]));
    }

    /* Emit object_id */
    if (count > 5) {
        emit_operand(operands[5], needs_extended_encoding(operands[5]));
    }

    /* Emit state (7th operand, only for OP_DEF_FIELD_ST) */
    if (count >= 7) {
        emit_operand(operands[6], needs_extended_encoding(operands[6]));
    }

    /* Emit lo_hi flag byte - always 0x01 per VM implementation */
    emit_byte(0x01);
}

/*
 * Generate GOTO_DEPENDING_ON instruction
 * Syntax: GOTO_DEPENDING_ON index, label1, label2, ...
 * Encoding: opcode (0x4B), index operand, count byte, then count * 2-byte offsets
 */
void gen_goto_depending_on(AstNode *node) {
    if (node->child_count < 2) return;

    AstNode *index_op = node->children[0];
    int label_count = node->child_count - 1;

    /* Emit opcode (potentially with complex mode for index operand) */
    AstNode *operands[2] = {index_op, NULL};
    bool complex = check_complex_mode(operands, 1);

    if (complex) {
        emit_byte(OP_GO_DEP | OP_COMPLEX);
        emit_byte(compute_mode_byte(operands, 1));
    } else {
        emit_byte(OP_GO_DEP);
    }

    /* Emit index operand */
    emit_operand(index_op, needs_extended_encoding(index_op));

    /* Emit label count */
    emit_byte((uint8_t)label_count);

    /* Emit offsets for each label.
     * Record patch positions for all offsets first, then resolve.
     * Forward offsets are relative to the position after each offset.
     * Backward offsets are relative to the end of the entire instruction
     * (matching original compiler behavior). */
    int *patch_positions = malloc(label_count * sizeof(int));
    const char **label_names = malloc(label_count * sizeof(char *));

    for (int i = 0; i < label_count; i++) {
        AstNode *label_node = node->children[i + 1];
        label_names[i] = (label_node->kind == AST_IDENT)
            ? label_node->data.ident.name : NULL;
        patch_positions[i] = emit_get_offset();
        emit_word_be(0);  /* Placeholder */
    }

    int instr_end = emit_get_offset();

    for (int i = 0; i < label_count; i++) {
        if (!label_names[i]) continue;
        int target_offset = lookup_label(label_names[i]);
        if (target_offset >= 0) {
            int pos_after = patch_positions[i] + 2;
            int base = (target_offset < pos_after) ? instr_end : pos_after;
            int rel = target_offset - base;
            emit_patch_word_be(patch_positions[i], (int16_t)rel);
        } else {
            /* Forward reference - always relative to pos_after */
            emit_add_forward_ref(patch_positions[i], label_names[i], current_proc_name);
        }
    }

    free(patch_positions);
    free(label_names);
}

/*
 * Generate code for procedure call
 */
void gen_proc_call(AstNode *node) {
    const char *name = node->data.call.name;
    int arg_count = node->child_count;

    bool complex = false;
    for (int i = 0; i < arg_count; i++) {
        if (needs_extended_encoding(node->children[i])) {
            complex = true;
            break;
        }
    }

    if (complex) {
        emit_byte(OP_CALL | OP_COMPLEX);
        /* Mode bytes */
        int mode_bytes = (arg_count + 7) / 8;
        for (int b = 0; b < mode_bytes; b++) {
            uint8_t mode = 0;
            for (int i = 0; i < 8 && (b * 8 + i) < arg_count; i++) {
                if (needs_extended_encoding(node->children[b * 8 + i])) {
                    mode |= (1 << (7 - i));
                }
            }
            emit_byte(mode);
        }
    } else {
        emit_byte(OP_CALL);
    }

    emit_byte(arg_count);

    for (int i = 0; i < arg_count; i++) {
        emit_operand(node->children[i], needs_extended_encoding(node->children[i]));
    }

    /* Emit relative offset to target procedure (same as JUMP).
     * Called procs must be defined after the caller in source order,
     * so all CALL offsets are forward (positive). */
    int patch_pos = emit_get_offset();
    emit_word_be(0);

    int proc_offset = lookup_proc(name);
    if (proc_offset >= 0) {
        int rel = proc_offset - (patch_pos + 2);
        emit_patch_word_be(patch_pos, (int16_t)rel);
    } else {
        /* Forward reference - use proc name */
        emit_add_forward_ref(patch_pos, name, NULL);
    }
}

/*
 * Verb handler function type for complex verbs
 */
typedef void (*VerbHandler)(AstNode *node, AstNode **operands, int count);

/*
 * Dispatch table entry for verb-to-opcode mapping
 *
 * operand_count semantics:
 *   >= 1  : fixed operand count, use emit_instruction()
 *    0    : no operands, emit opcode byte only (when handler is NULL)
 *   -1    : variable operand count, use emit_var_instruction() with node's count
 *   -2    : use emit_instruction() with node's count (no count byte)
 *
 * If handler is non-NULL, it is called instead of the default dispatch.
 */
typedef struct {
    const char *name;
    uint8_t opcode;
    int operand_count;
    VerbHandler handler;
} VerbEntry;

/* Complex verb handlers */

static void handle_move(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 2) {
        /* Check if both operands are same-size arrays -> emit MOVE_BLOCK.
         * The original compiler emits MOVE_BLOCK (0x62) for array-to-array MOVE:
         *   opcode, count_byte, src_operand, dst_operand
         * Sema has already validated that sizes match. */
        int array_size = 0;
        if (operands[0] && operands[0]->kind == AST_IDENT &&
            operands[1] && operands[1]->kind == AST_IDENT) {
            Symbol *src = symtab_lookup_var(operands[0]->data.ident.name);
            if (src && src->data.var.array_size > 0) {
                array_size = src->data.var.array_size;
            }
        }

        if (array_size > 0) {
            /* Emit MOVE_BLOCK: opcode, count, src, dst */
            bool complex = check_complex_mode(operands, 2);
            if (complex) {
                emit_byte(OP_MOVE_BLOCK | OP_COMPLEX);
                emit_byte(compute_mode_byte(operands, 2));
            } else {
                emit_byte(OP_MOVE_BLOCK);
            }
            emit_byte((uint8_t)array_size);
            emit_operand(operands[0], needs_extended_encoding(operands[0]));
            emit_operand(operands[1], needs_extended_encoding(operands[1]));
        } else {
            emit_instruction(OP_MOVE, operands, 2);
        }
    } else if (count == 3) {
        emit_instruction(OP_MOVE_BLOCK, operands, 3);
    }
}

static void handle_divide(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 2)
        emit_instruction(OP_DIV, operands, 2);
    else
        emit_instruction(OP_DIV_REM, operands, 3);
}

static void handle_clear(AstNode *node, AstNode **operands, int count) {
    (void)node;
    /* CLEAR 0x51 = single operand with count byte
     * CLEAR_RANGE 0x52 = exactly 2 operands, standard encoding
     * Special: if single operand is a structure name, expand to (start_slot, count)
     * Special: if single operand is an array variable, use array dimension as count */
    if (count == 2) {
        emit_range_instruction(OP_CLEAR_RANGE, operands, 2);
    } else if (count == 1 && operands[0] && operands[0]->kind == AST_IDENT) {
        /* Check if this is a structure name */
        StructureGroup *sg = symtab_lookup_structure(operands[0]->data.ident.name);
        if (sg) {
            emit_struct_clear_save(OP_CLEAR, sg->start_slot, sg->count);
        } else {
            /* Not a structure - check if it's an array variable */
            Symbol *sym = symtab_lookup_var(operands[0]->data.ident.name);
            if (sym && sym->data.var.array_size > 0) {
                int slot = VAR_SLOT(sym);
                emit_struct_clear_save(OP_CLEAR, slot, sym->data.var.array_size);
            } else {
                emit_var_instruction(OP_CLEAR, operands, count);
            }
        }
    } else {
        emit_var_instruction(OP_CLEAR, operands, count);
    }
}

static void handle_exit(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_EXIT);
    else
        emit_instruction(OP_EXIT_RC, operands, 1);
}

static void handle_return(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_RETURN);
    else
        emit_instruction(OP_RETURN_RC, operands, 1);
}

static void handle_fetch(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 1)
        emit_instruction(OP_FETCH, operands, 1);
    else
        emit_instruction(OP_FETCH_RQ, operands, 2);
}

static void handle_close(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_CLOSE_ALL);
    else
        emit_instruction(OP_CLOSE, operands, 1);
}

static void handle_close_window(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_CLOSE_WINDOW);
    else
        emit_instruction(OP_CLOSE_OPEN_WIN, operands, 1);
}

static void handle_open_error_window(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_OPEN_ERR_WIN2);
    else if (count == 1)
        emit_instruction(OP_OPEN_ERR_WIN2, operands, 1);
    else
        emit_instruction(OP_OPEN_ERR_WIN3, operands, 2);
}

static void handle_send(AstNode *node, AstNode **operands, int count) {
    /* SEND encoding:
     * 1 operand: opcode(14) + op1 + timeout(2 LE) + flags(1)
     * 2 operands: opcode(15) + op1 + op2 + timeout(2 LE) + flags(1)
     */
    int16_t timeout = node->data.call.send_timeout;
    uint8_t flags = node->data.call.send_flags;

    if (count == 1) {
        bool complex = check_complex_mode(operands, 1);
        if (complex) {
            emit_byte(OP_SEND | OP_COMPLEX);
            emit_byte(compute_mode_byte(operands, 1));
        } else {
            emit_byte(OP_SEND);
        }
        emit_operand(operands[0], needs_extended_encoding(operands[0]));
    } else {
        bool complex = check_complex_mode(operands, 2);
        if (complex) {
            emit_byte(OP_SEND_ID | OP_COMPLEX);
            emit_byte(compute_mode_byte(operands, 2));
        } else {
            emit_byte(OP_SEND_ID);
        }
        emit_operand(operands[0], needs_extended_encoding(operands[0]));
        emit_operand(operands[1], needs_extended_encoding(operands[1]));
    }
    /* Emit timeout as 2-byte little-endian */
    emit_byte((uint8_t)(timeout & 0xFF));
    emit_byte((uint8_t)((timeout >> 8) & 0xFF));
    /* Emit flags byte */
    emit_byte(flags);
}

static void handle_save(AstNode *node, AstNode **operands, int count) {
    (void)node;
    /* SAVE 0x4D = var_count byte + 2 operands (name, slot)
     * SAVE_RANGE 0x4E = 3 operands (name, start_slot, end_slot)
     * Special: if single operand is a structure name, expand to (start_slot, count) */
    if (count == 2) {
        /* SAVE name, slot - emit SAVE (0x4D) with var_count = 1 */
        bool complex = check_complex_mode(operands, 2);
        if (complex) {
            emit_byte(OP_SAVE | OP_COMPLEX);
            emit_byte(compute_mode_byte(operands, 2));
        } else {
            emit_byte(OP_SAVE);
        }
        emit_byte(1);  /* var_count = 1 (save 1 slot) */
        emit_operand(operands[0], needs_extended_encoding(operands[0]));
        emit_operand(operands[1], needs_extended_encoding(operands[1]));
    } else if (count == 3) {
        /* SAVE name, start, end - emit SAVE_RANGE (0x4E)
         * End operand uses array-end resolution */
        emit_range_instruction(OP_SAVE_RANGE, operands, 3);
    } else if (count == 1 && operands[0] && operands[0]->kind == AST_IDENT) {
        /* Check if this is a structure name */
        StructureGroup *sg = symtab_lookup_structure(operands[0]->data.ident.name);
        if (sg) {
            emit_struct_clear_save(OP_SAVE, sg->start_slot, sg->count);
        } else {
            /* Not a structure - check if it's an array variable */
            Symbol *sym = symtab_lookup_var(operands[0]->data.ident.name);
            if (sym && sym->data.var.array_size > 0) {
                int slot = VAR_SLOT(sym);
                emit_struct_clear_save(OP_SAVE, slot, sym->data.var.array_size);
            } else {
                emit_var_instruction(OP_SAVE, operands, count);
            }
        }
    } else {
        emit_var_instruction(OP_SAVE, operands, count);
    }
}

static void handle_sound(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 0)
        emit_byte(OP_SOUND);
    else
        emit_instruction(OP_SOUND_ARGS, operands, 2);
}

static void handle_set_key(AstNode *node, AstNode **operands, int count) {
    (void)node;
    if (count == 3)
        emit_instruction(OP_SET_KEY, operands, 3);
    else
        emit_instruction(OP_SET_KEY_PGM, operands, 5);
}

static void handle_set_function(AstNode *node, AstNode **operands, int count) {
    (void)operands; (void)count;
    gen_set_function(node);
}

static void handle_set_attribute(AstNode *node, AstNode **operands, int count) {
    (void)operands; (void)count;
    gen_set_attribute(node);
}

static void handle_make_format(AstNode *node, AstNode **operands, int count) {
    (void)operands; (void)count;
    gen_make_format(node);
}

static void handle_define_field(AstNode *node, AstNode **operands, int count) {
    (void)operands; (void)count;
    gen_define_field(node);
}

static void handle_goto_depending_on(AstNode *node, AstNode **operands, int count) {
    (void)operands; (void)count;
    gen_goto_depending_on(node);
}

/* Opcodes 0x3C-0x3D (SYNC_SAVE, SYNC_RELEASE), 0x3E (TIMER_ON), 0x61 (TIMER_OFF),
 * 0x5F (ACCESS), 0x5D-0x5E (UPLOAD, DOWNLOAD), 0x5B-0x5C (FILE_SCREEN, SHOW_SCREEN),
 * 0x59 (SET_BACKGROUND), 0x68 (TRACK), 0x69 (LOG), 0x60 (NOP):
 * These opcodes appear in the TBOL patent interpreter source (v6) but were never
 * fully implemented -- stubbed with empty function bodies or no implementation at all.
 * Zero usage in the 203-file Prodigy production corpus. Not in the TBOLCV 4.0 converter. */

/*
 * READ/WRITE handler: selects opcode based on operand count.
 *   2 operands -> OP_READ  (0x0E) / OP_WRITE  (0x10) - line-oriented
 *   3 operands -> OP_READ3 (0x0F) / OP_WRITE3 (0x11) - fixed-width record
 */
static void handle_read(AstNode *node, AstNode **operands, int count) {
    (void)node;
    uint8_t opcode = (count == 3) ? OP_READ3 : OP_READ;
    emit_instruction(opcode, operands, count);
}

static void handle_write(AstNode *node, AstNode **operands, int count) {
    (void)node;
    uint8_t opcode = (count == 3) ? OP_WRITE3 : OP_WRITE;
    emit_instruction(opcode, operands, count);
}

/*
 * Static dispatch table for verb-to-opcode mapping.
 * Simple verbs use opcode + operand_count; complex verbs use a handler.
 */
static const VerbEntry verb_table[] = {
    /* Simple fixed-operand verbs */
    { "MOVE_ABS",         OP_MOVE_ABS,    2,  NULL },
    { "ADD",              OP_ADD,          2,  NULL },
    { "SUBTRACT",         OP_SUB,          2,  NULL },
    { "MULTIPLY",         OP_MUL,          2,  NULL },
    { "AND",              OP_AND,          2,  NULL },
    { "OR",               OP_OR,           2,  NULL },
    { "XOR",              OP_XOR,          2,  NULL },
    { "TEST",             OP_TEST,         2,  NULL },
    { "SWAP",             OP_SWAP,         2,  NULL },
    { "FILL",             OP_FILL,         3,  NULL },
    { "PUSH",             OP_PUSH,         1,  NULL },
    { "POP",              OP_POP,          1,  NULL },
    { "SUBSTR",           OP_SUBSTR,       4,  NULL },
    { "INSTR",            OP_INSTR,        3,  NULL },
    { "UPPERCASE",        OP_UPPER,        1,  NULL },
    { "LENGTH",           OP_LENGTH,       2,  NULL },
    { "FORMAT",           OP_FORMAT,       3,  NULL },
    { "OPEN_WINDOW",      OP_OPEN_WINDOW,  1,  NULL },
    { "KILL",             OP_KILL,        -2,  NULL },
    { "OPEN",             OP_OPEN,         2,  NULL },
    { "NOTE",             OP_NOTE,         2,  NULL },
    { "POINT",            OP_POINT,        2,  NULL },
    { "DELETE",           OP_DELETE,        1,  NULL },
    { "CONNECT",          OP_CONNECT,      1,  NULL },
    { "RECEIVE",          OP_RECEIVE,      2,  NULL },
    { "CANCEL",           OP_CANCEL,      -2,  NULL },
    { "RESTORE",          OP_RESTORE,      2,  NULL },
    { "RELEASE",          OP_RELEASE,      1,  NULL },
    { "ERASE",            OP_ERASE,       -2,  NULL },
    { "SET_CURSOR",       OP_SET_CURSOR,  -2,  NULL },
    { "STOP",             OP_STOP,        -2,  NULL },
    { "ERROR",            OP_ERROR,        1,  NULL },
    { "TRIGGER_FUNCTION", OP_TRIG_FUNC,    1,  NULL },
    { "SORT",             OP_SORT,         3,  NULL },
    { "LOOKUP",           OP_LOOKUP,       5,  NULL },

    /* Variable-operand verbs (emit_var_instruction with count byte) */
    { "STRING",           OP_STRING,      -1,  NULL },
    { "EDIT",             OP_EDIT,        -1,  NULL },
    { "LINK",             OP_LINK,        -1,  NULL },
    { "TRANSFER",         OP_TRANSFER,    -1,  NULL },

    /* Pass-through count verbs (emit_instruction with node's count, no count byte) */
    { "NAVIGATE",         OP_NAVIGATE,    -2,  NULL },

    /* Opcode-only verbs (no operands) */
    { "NAVIGATE_FIRST",   OP_NAV_FIRST,    0,  NULL },
    { "NAVIGATE_NEXT",    OP_NAV_NEXT,     0,  NULL },
    { "NAVIGATE_BACK",    OP_NAV_BACK,     0,  NULL },
    { "NAVIGATE_LAST",    OP_NAV_LAST,     0,  NULL },
    { "PURGE_CACHE",      OP_PURGE,        0,  NULL },
    { "REFRESH",          OP_REFRESH,      0,  NULL },
    { "WAIT",             OP_WAIT,         0,  NULL },
    { "DISCONNECT",       OP_DISCONNECT,   0,  NULL },

    /* Complex verbs with custom handlers */
    { "MOVE",             0,               0,  handle_move },
    { "DIVIDE",           0,               0,  handle_divide },
    { "CLEAR",            0,               0,  handle_clear },
    { "SAVE",             0,               0,  handle_save },
    { "SEND",             0,               0,  handle_send },
    { "EXIT",             0,               0,  handle_exit },
    { "RETURN",           0,               0,  handle_return },
    { "CLOSE",            0,               0,  handle_close },
    { "CLOSE_WINDOW",     0,               0,  handle_close_window },
    { "FETCH",            0,               0,  handle_fetch },
    { "OPEN_ERROR_WINDOW",0,               0,  handle_open_error_window },
    { "SOUND",            0,               0,  handle_sound },
    { "SET_KEY",          0,               0,  handle_set_key },
    { "SET_FUNCTION",     0,               0,  handle_set_function },
    { "SET_ATTRIBUTE",    0,               0,  handle_set_attribute },
    { "MAKE_FORMAT",      0,               0,  handle_make_format },
    { "DEFINE_FIELD",     0,               0,  handle_define_field },
    { "GOTO_DEPENDING_ON",0,               0,  handle_goto_depending_on },
    { "READ",             0,               0,  handle_read },
    { "WRITE",            0,               0,  handle_write },
};

#define VERB_TABLE_SIZE (sizeof(verb_table) / sizeof(verb_table[0]))

void gen_verb(AstNode *node) {
    const char *verb = node->data.call.name;
    int count = node->child_count;
    AstNode **operands = node->children;

    for (size_t i = 0; i < VERB_TABLE_SIZE; i++) {
        if (strcasecmp(verb, verb_table[i].name) != 0)
            continue;

        /* Custom handler takes precedence */
        if (verb_table[i].handler) {
            verb_table[i].handler(node, operands, count);
            return;
        }

        int nops = verb_table[i].operand_count;

        if (nops == 0) {
            /* Opcode-only */
            emit_byte(verb_table[i].opcode);
        } else if (nops == -1) {
            /* Variable operand count with count byte */
            emit_var_instruction(verb_table[i].opcode, operands, count);
        } else if (nops == -2) {
            /* Pass-through count, no count byte */
            emit_instruction(verb_table[i].opcode, operands, count);
        } else {
            /* Fixed operand count */
            emit_instruction(verb_table[i].opcode, operands, nops);
        }
        return;
    }

    /* Unknown verb - emit NOP */
    emit_byte(OP_BREAK);
}
