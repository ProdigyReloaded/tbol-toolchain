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
 * tboldasm - TBOL bytecode disassembler
 *
 * Produces TBOL-like source from .COD or .PGM bytecode files.
 * Output includes procedure boundaries, named procedure calls,
 * and optional GEV name resolution.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "bytecode/ir.h"
#include "bytecode/gev.h"
#include "bytecode/symtab_analysis.h"
#include "bytecode/cod_file.h"
#include "bytecode/decode.h"
#include "source_header.h"

/* ── Command-line usage ─────────────────────────────────────────────── */

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <input.cod|input.pgm>\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -r, --raw        Raw listing (addresses + mnemonics, no structure)\n");
    fprintf(stderr, "  -t, --tabular    Tabular format with hex bytes column\n");
    fprintf(stderr, "  -d, --data       Emit inferred DATA section and COPY XXCGTSYS\n");
    fprintf(stderr, "  -I <path>        Include path for GEV name resolution (XXCGTSYS)\n");
    fprintf(stderr, "  -h, --help       Show this help\n");
}

/* ── String buffer ──────────────────────────────────────────────────── */

typedef struct {
    char *data;
    int len;
    int cap;
} StringBuf;

static void sbuf_init(StringBuf *sb) {
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

static void sbuf_reset(StringBuf *sb) {
    sb->len = 0;
    if (sb->data) sb->data[0] = '\0';
}

static void sbuf_free(StringBuf *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

__attribute__((format(printf, 2, 3)))
static void sbuf_printf(StringBuf *sb, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int needed = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    while (sb->len + needed + 1 > sb->cap) {
        sb->cap = sb->cap ? sb->cap * 2 : 256;
        sb->data = realloc(sb->data, sb->cap);
    }
    va_start(ap, fmt);
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, ap);
    va_end(ap);
    sb->len += needed;
}

/* ── Target collection (jump/call targets for labels and procs) ───── */

typedef struct {
    uint16_t addr;
    char *name;
    bool is_proc;
} Target;

typedef struct {
    Target *items;
    int count;
    int capacity;
} TargetList;

/* Returns true if a new target was added, false if it already existed. */
static bool target_add(TargetList *t, uint16_t addr, bool is_proc, const char *name) {
    for (int i = 0; i < t->count; i++) {
        if (t->items[i].addr == addr) {
            /* A CALL target upgrades a jump label to a proc boundary */
            if (is_proc && !t->items[i].is_proc) {
                t->items[i].is_proc = true;
                free(t->items[i].name);
                t->items[i].name = strdup(name);
            }
            return false;
        }
    }
    if (t->count >= t->capacity) {
        t->capacity = t->capacity ? t->capacity * 2 : 64;
        t->items = realloc(t->items, t->capacity * sizeof(Target));
        if (!t->items) { fprintf(stderr, "out of memory\n"); exit(1); }
    }
    t->items[t->count].addr = addr;
    t->items[t->count].name = strdup(name);
    t->items[t->count].is_proc = is_proc;
    t->count++;
    return true;
}

static Target *target_find(TargetList *t, uint16_t addr) {
    for (int i = 0; i < t->count; i++) {
        if (t->items[i].addr == addr) return &t->items[i];
    }
    return NULL;
}

static int cmp_target(const void *a, const void *b) {
    uint16_t va = ((const Target *)a)->addr;
    uint16_t vb = ((const Target *)b)->addr;
    return (va > vb) - (va < vb);
}

static void assign_names(TargetList *t) {
    /* Names are assigned during collection (encounter order).
     * Sort by address for efficient lookup. */
    qsort(t->items, t->count, sizeof(Target), cmp_target);
}

static void targets_free(TargetList *t) {
    for (int i = 0; i < t->count; i++) free(t->items[i].name);
    free(t->items);
}

static void collect_targets(Program *prog, TargetList *targets) {
    int proc_num = 1;
    char buf[32];

    /* Main procedure always starts at address 0 */
    target_add(targets, 0, true, "main");

    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        if (instr->mnemonic == MNEM_CALL) {
            /* Jump offset 0 = unresolved (undefined proc) — don't register */
            if (instr->has_jump && instr->jump_offset != 0) {
                snprintf(buf, sizeof(buf), "proc_%d", proc_num);
                if (target_add(targets, instr->jump_target, true, buf))
                    proc_num++;
            }
        } else if (instr->has_jump) {
            snprintf(buf, sizeof(buf), "label_%d", instr->jump_target);
            target_add(targets, instr->jump_target, false, buf);
        }

        /* GDO operands 1..N are absolute target addresses stored as literals */
        if (instr->mnemonic == MNEM_GOTO_DEPENDING_ON) {
            for (int i = 1; i < instr->operand_count; i++) {
                Operand *op = &instr->operands[i];
                uint16_t target = (op->kind == OP_LITERAL_STR && op->str_value)
                    ? (uint16_t)atoi(op->str_value) : (uint16_t)op->value;
                snprintf(buf, sizeof(buf), "label_%d", target);
                target_add(targets, target, false, buf);
            }
        }
    }

    assign_names(targets);
}

/* ── Function key and action name tables ────────────────────────────── */

static const char *function_names[] = {
    [0] = "NO_EVENT", [1] = "ADD_TEXT", [2] = "ADD_TEXT_AND_FIELD_END",
    [3] = "BACKSPACE", [4] = "DELETE_TEXT", [5] = "TOGGLE_INSERT",
    [6] = "HELP", [7] = "INSERT_TEXT", [8] = "FIELD_HELP",
    [9] = "PAGE_HELP", [10] = "NAVIGATION", [11] = "INTERFIELD_CURSOR",
    [12] = "NEXT", [13] = "BACK", [14] = "PATH", [15] = "JUMP",
    [16] = "ACTION", [17] = "FIELD_END", [18] = "ELEMENT_END",
    [19] = "PAGE_END", [20] = "BYE", [21] = "LOGON", [22] = "VIEWPATH",
    [23] = "KEYWORD", [24] = "GUIDE", [25] = "RECALL", [26] = "UNDO",
    [27] = "WHERE", [28] = "SRNPRT", [29] = "SMFILE", [30] = "SCAN",
    [31] = "TOOLS", [32] = "DIRECTORY", [33] = "INDEX", [34] = "FIND",
    [35] = "LEAVE", [36] = "ZIP", [37] = "NEW_LINE",
    [38] = "SCROLL_UP", [39] = "SCROLL_DOWN",
    [40] = "SET_CLOSE_WINDOW", [41] = "SET_OPEN_WINDOW",
    [42] = "CURSOR", [43] = "LOGOFF", [44] = "LOOK", [45] = "TRAVEL",
    [46] = "FIRST_PAGE", [47] = "LAST_PAGE", [48] = "PREVIOUS_MENU",
    [49] = "LOGICAL_FUNCTIONS",
};
#define FUNCTION_NAME_COUNT ((int)(sizeof(function_names)/sizeof(function_names[0])))

static const char *resolve_function(int val) {
    if (val >= 0 && val < FUNCTION_NAME_COUNT && function_names[val])
        return function_names[val];
    return NULL;
}

static const char *resolve_action(int val) {
    switch (val) {
        case 0:  return "NORMAL";
        case 16: return "DISABLE";
        case 32: return "FILTER";
        case 64: return "FILTER_ON";
        default: return NULL;
    }
}

/* ── Context-aware operand formatting ───────────────────────────────── */

/*
 * Determine if operand at position `idx` in instruction `instr` should
 * be displayed as an unquoted integer based on instruction semantics.
 */
static bool should_show_numeric(Instruction *instr, int idx) {
    Mnemonic m = instr->mnemonic;

    if (m == MNEM_ADD || m == MNEM_SUB || m == MNEM_MUL ||
        m == MNEM_DIV || m == MNEM_FILL) {
        return true;
    }

    /* TBOL bytecode order is (src, dest) — idx 0 is source, idx 1 is dest */
    if ((m == MNEM_MOVE || m == MNEM_MOVE_ABS || m == MNEM_SWAP) && idx == 0) {
        if (instr->operand_count >= 2) {
            OperandKind dest = instr->operands[1].kind;
            if (dest == OP_REG_I || dest == OP_REG_D) return true;
        }
    }

    if (m == MNEM_SUBSTR && (idx == 2 || idx == 3)) return true;

    if (m >= MNEM_CJEQ && m <= MNEM_CJGE && idx < 2) {
        Operand *other = &instr->operands[idx == 0 ? 1 : 0];
        if (other->kind == OP_REG_I || other->kind == OP_REG_D) return true;
    }

    return false;
}

static char *format_operand(Operand *op, GEVTable *gev, Instruction *instr, int idx) {
    if (gev && op->kind == OP_GEV) {
        const char *name = gev_lookup(gev, op->value);
        if (name) return strdup(name);
    }
    if (instr && should_show_numeric(instr, idx) && operand_is_numeric_literal(op)) {
        return operand_to_numeric_string(op);
    }
    return operand_to_string(op);
}

/* Helper: emit all operands as a comma-separated list */
static void emit_operand_list(StringBuf *sb, Instruction *instr, GEVTable *gev) {
    for (int i = 0; i < instr->operand_count; i++) {
        sbuf_printf(sb, "%s", i == 0 ? " " : ", ");
        char *s = format_operand(&instr->operands[i], gev, instr, i);
        sbuf_printf(sb, "%s", s);
        free(s);
    }
}

/* Helper: resolve a jump target address to a label name */
static const char *resolve_target(TargetList *targets, uint16_t addr) {
    Target *t = target_find(targets, addr);
    return (t && t->name) ? t->name : "???";
}

/* ── Instruction formatters ─────────────────────────────────────────── */

/* Each formatter returns true if it handled the instruction. */

typedef struct {
    StringBuf *sb;
    TargetList *targets;
    GEVTable *gev;
    bool raw;
} FormatCtx;

static bool fmt_call(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_CALL || !instr->has_jump) return false;

    /* Jump offset 0 = unresolved call (undefined proc) */
    const char *name = (instr->jump_offset == 0)
        ? "_undefined" : resolve_target(ctx->targets, instr->jump_target);
    sbuf_printf(ctx->sb, "%s", name);
    if (instr->operand_count > 0)
        emit_operand_list(ctx->sb, instr, ctx->gev);
    sbuf_printf(ctx->sb, ";");
    return true;
}

static bool fmt_return_rc(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_RETURN || instr->operand_count == 0) return false;

    char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
    sbuf_printf(ctx->sb, "RETURN %s;", s);
    free(s);
    return true;
}

static bool fmt_exit_rc(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_EXIT || instr->operand_count == 0) return false;

    char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
    sbuf_printf(ctx->sb, "EXIT %s;", s);
    free(s);
    return true;
}

static bool fmt_send(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_SEND) return false;

    sbuf_printf(ctx->sb, "SEND");
    emit_operand_list(ctx->sb, instr, ctx->gev);

    /* SEND has 3 trailing bytes: 2-byte LE timeout + 1-byte flags.
     * Flag bits: 0x04 = PRIORITY, 0x02 = OPT_HDRS */
    bool has_mod = false;
    if (instr->send_timeout != 0) {
        sbuf_printf(ctx->sb, ", TIMEOUT(%d)", instr->send_timeout);
        has_mod = true;
    }
    if (instr->send_flags & 0x04) {
        sbuf_printf(ctx->sb, "%sPRIORITY", has_mod || instr->operand_count > 0 ? ", " : " ");
        has_mod = true;
    }
    if (instr->send_flags & 0x02) {
        sbuf_printf(ctx->sb, "%sOPT_HDRS", has_mod || instr->operand_count > 0 ? ", " : " ");
    }
    sbuf_printf(ctx->sb, ";");
    return true;
}

static bool fmt_set_attribute(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_SET_ATTRIBUTE || instr->operand_count < 5)
        return false;

    char *field_s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
    sbuf_printf(ctx->sb, "SET_ATTRIBUTE %s, '", field_s);
    free(field_s);

    /* Operands 1-4 are the 4 attribute bytes from the bytecode:
     *   [1] state:  field state (0x80=ACTION, 0x40=DISPLAY, 0x20=INPUT,
     *               0x08=PP_NOT_FIRED)
     *   [2] format: field format (0x80=ALPHA, 0x40=NUMERIC, 0x10=PASSWORD)
     *   [3] fg:     foreground color (0x80 = default)
     *   [4] bg:     background color (0x80 = default) */
    uint8_t state = (uint8_t)instr->operands[1].value;
    uint8_t form = (uint8_t)instr->operands[2].value;
    uint8_t fg = (uint8_t)instr->operands[3].value;
    uint8_t bg = (uint8_t)instr->operands[4].value;

    switch (state & 0xF0) {
        case 0x80: sbuf_printf(ctx->sb, "ACTION "); break;
        case 0x40: sbuf_printf(ctx->sb, "DISPLAY "); break;
        case 0x20: sbuf_printf(ctx->sb, "INPUT "); break;
        default: break;
    }
    if (state & 0x08) sbuf_printf(ctx->sb, "PP_NOT_FIRED ");
    if (form & 0x80) sbuf_printf(ctx->sb, "ALPHABETIC ");
    if (form & 0x40) sbuf_printf(ctx->sb, "NUMERIC ");
    if (form & 0x10) sbuf_printf(ctx->sb, "PASSWORD ");
    if (fg != 0x80 || bg != 0x80) sbuf_printf(ctx->sb, "COLOR(%d,%d)", fg, bg);
    /* Trim trailing space before closing quote */
    while (ctx->sb->len > 0 && ctx->sb->data[ctx->sb->len - 1] == ' ')
        ctx->sb->data[--ctx->sb->len] = '\0';
    sbuf_printf(ctx->sb, "';");
    return true;
}

static int operand_int_value(Operand *op) {
    if (op->kind == OP_LITERAL_STR && op->str_value && op->str_len > 0)
        return atoi(op->str_value);
    if (op->kind == OP_LITERAL_NUM)
        return op->value;
    return -1;
}

static bool fmt_make_format(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_MAKE_FORMAT || instr->operand_count < 4)
        return false;

    char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
    sbuf_printf(ctx->sb, "MAKE_FORMAT %s,", s);
    free(s);

    /* After the dest operand, specs come in triplets: field, fixed_width, embed_width.
     * Output as colon syntax: field:fixed:embed */
    for (int i = 1; i + 2 < instr->operand_count; i += 3) {
        sbuf_printf(ctx->sb, "\n");
        char *field = format_operand(&instr->operands[i], ctx->gev, instr, i);
        sbuf_printf(ctx->sb, "%s", field);
        free(field);

        int fix_val = operand_int_value(&instr->operands[i + 1]);
        int emb_val = operand_int_value(&instr->operands[i + 2]);

        if (fix_val > 0 && emb_val > 0) sbuf_printf(ctx->sb, ":%d:%d", fix_val, emb_val);
        else if (fix_val > 0) sbuf_printf(ctx->sb, ":%d", fix_val);
        else if (emb_val > 0) sbuf_printf(ctx->sb, "::%d", emb_val);

        if (i + 5 < instr->operand_count) sbuf_printf(ctx->sb, ",");
    }
    sbuf_printf(ctx->sb, ";");
    return true;
}

static bool fmt_set_function(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_SET_FUNCTION || instr->operand_count < 2)
        return false;

    /* Operand 0: function key ID, Operand 1: action */
    int func_val = operand_int_value(&instr->operands[0]);
    int action_val = operand_int_value(&instr->operands[1]);

    const char *func_name = resolve_function(func_val);
    const char *action_name = resolve_action(action_val);

    sbuf_printf(ctx->sb, "SET_FUNCTION ");
    if (func_name)
        sbuf_printf(ctx->sb, "%s", func_name);
    else {
        char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
        sbuf_printf(ctx->sb, "%s", s);
        free(s);
    }

    sbuf_printf(ctx->sb, ", ");
    if (action_name)
        sbuf_printf(ctx->sb, "%s", action_name);
    else {
        char *s = format_operand(&instr->operands[1], ctx->gev, instr, 1);
        sbuf_printf(ctx->sb, "%s", s);
        free(s);
    }

    /* Remaining operands (program name, params) */
    for (int i = 2; i < instr->operand_count; i++) {
        sbuf_printf(ctx->sb, ", ");
        char *s = format_operand(&instr->operands[i], ctx->gev, instr, i);
        sbuf_printf(ctx->sb, "%s", s);
        free(s);
    }
    sbuf_printf(ctx->sb, ";");
    return true;
}

static bool fmt_trigger_function(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_TRIG_FUNC || instr->operand_count < 1)
        return false;

    int func_val = operand_int_value(&instr->operands[0]);
    const char *func_name = resolve_function(func_val);

    sbuf_printf(ctx->sb, "TRIGGER_FUNCTION ");
    if (func_name)
        sbuf_printf(ctx->sb, "%s", func_name);
    else {
        char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
        sbuf_printf(ctx->sb, "%s", s);
        free(s);
    }
    sbuf_printf(ctx->sb, ";");
    return true;
}

static bool fmt_move_abs(FormatCtx *ctx, Instruction *instr) {
    if (ctx->raw || instr->mnemonic != MNEM_MOVE_ABS) return false;

    sbuf_printf(ctx->sb, "MOVE");
    emit_operand_list(ctx->sb, instr, ctx->gev);
    sbuf_printf(ctx->sb, ", ABS;");
    return true;
}

static bool fmt_goto_depending_on(FormatCtx *ctx, Instruction *instr) {
    if (instr->mnemonic != MNEM_GOTO_DEPENDING_ON) return false;

    sbuf_printf(ctx->sb, "%s", instr->mnem_str);
    if (instr->operand_count > 0) {
        char *s = format_operand(&instr->operands[0], ctx->gev, instr, 0);
        sbuf_printf(ctx->sb, " %s", s);
        free(s);
    }
    for (int i = 1; i < instr->operand_count; i++) {
        Operand *op = &instr->operands[i];
        uint16_t addr = (op->kind == OP_LITERAL_STR && op->str_value)
            ? (uint16_t)atoi(op->str_value) : (uint16_t)op->value;
        if (ctx->raw)
            sbuf_printf(ctx->sb, ", @%04X", addr);
        else
            sbuf_printf(ctx->sb, ", %s", resolve_target(ctx->targets, addr));
    }
    sbuf_printf(ctx->sb, ";");
    return true;
}

/* Generic instruction — handles everything not caught by special formatters */
static void fmt_generic(FormatCtx *ctx, Instruction *instr) {
    sbuf_printf(ctx->sb, "%s", instr->mnem_str);

    /* MOVE_BLOCK (0x62) has a leading count byte — show it.
     * SAVE and CLEAR counts are implicit in the operands. */
    if (instr->var_count > 0 && instr->opcode == 0x62) {
        sbuf_printf(ctx->sb, " %d,", instr->var_count);
    }

    emit_operand_list(ctx->sb, instr, ctx->gev);

    if (instr->has_jump) {
        if (instr->operand_count > 0) sbuf_printf(ctx->sb, ",");
        if (ctx->raw)
            sbuf_printf(ctx->sb, " @%04X", instr->jump_target);
        else
            sbuf_printf(ctx->sb, " %s", resolve_target(ctx->targets, instr->jump_target));
    }
    sbuf_printf(ctx->sb, ";");
}

/* Format one instruction into the string buffer */
static void format_instruction(FormatCtx *ctx, Instruction *instr) {
    if (fmt_call(ctx, instr)) return;
    if (fmt_return_rc(ctx, instr)) return;
    if (fmt_exit_rc(ctx, instr)) return;
    if (fmt_send(ctx, instr)) return;
    if (fmt_set_attribute(ctx, instr)) return;
    if (fmt_set_function(ctx, instr)) return;
    if (fmt_trigger_function(ctx, instr)) return;
    if (fmt_make_format(ctx, instr)) return;
    if (fmt_move_abs(ctx, instr)) return;
    if (fmt_goto_depending_on(ctx, instr)) return;
    fmt_generic(ctx, instr);
}

/* ── Tabular hex + text interleaving ────────────────────────────────── */

/*
 * Column layout for tabular mode:
 *   Col 0-3:   "    " (indent)
 *   Col 4-7:   "XXXX" (address)
 *   Col 8-9:   "  " (gap)
 *   Col 10-33: hex bytes (8 * 3 = 24 chars)
 *   Col 34-39: "      " (gap)
 *   Col 40+:   instruction text
 */

static int print_hex_line(Program *prog, int start, int avail) {
    int n = avail < 8 ? avail : 8;
    for (int i = 0; i < n; i++)
        printf("%02X ", prog->code[start + i]);
    for (int i = n; i < 8; i++)
        printf("   ");
    return n;
}

/*
 * Print an instruction with hex bytes on the left and text on the right.
 * Hex bytes and text lines are interleaved row by row: each output row
 * shows up to 8 hex bytes alongside one line of instruction text.
 * Whichever side is longer gets blank padding on the shorter side.
 */
static void print_tabular_instruction(Program *prog, Instruction *instr, const char *text) {
    int hex_start = instr->address;
    int hex_total = instr->length;
    if (hex_start + hex_total > prog->code_size)
        hex_total = prog->code_size - hex_start;

    int hex_pos = 0;
    const char *p = text;
    bool first_text_line = true;

    while (*p || hex_pos < hex_total) {
        const char *eol = p;
        while (*eol && *eol != '\n') eol++;

        if (hex_pos < hex_total) {
            if (hex_pos == 0)
                printf("    %04X  ", instr->address);
            else
                printf("          ");
            int n = print_hex_line(prog, hex_start + hex_pos, hex_total - hex_pos);
            hex_pos += n;
        } else {
            printf("                                  ");
        }

        if (*p) {
            printf("%s", first_text_line ? "      " : "          ");
            first_text_line = false;
            printf("%.*s\n", (int)(eol - p), p);
            p = *eol ? eol + 1 : eol;
        } else {
            printf("\n");
        }
    }
}

/* ── Structured mode output ─────────────────────────────────────────── */

static void print_structured_instruction(const char *text) {
    const char *p = text;
    bool first = true;

    while (*p) {
        const char *eol = p;
        while (*eol && *eol != '\n') eol++;

        printf("%s", first ? "    " : "        ");
        first = false;
        printf("%.*s\n", (int)(eol - p), p);
        p = *eol ? eol + 1 : eol;
    }
}

/* ── Preamble prefix for tabular mode ───────────────────────────────── */

/* Aligns preamble text to the PROC column (col 40) */
#define TAB_PREFIX "                                       "

/* ── DATA section inference ─────────────────────────────────────────── */

static void print_data_section(Program *prog, const char *prefix) {
    SymbolTable *st = symbol_table_new();
    symbol_table_scan(st, prog);

    if (st->max_slot < 0) {
        symbol_table_free(st);
        return;
    }

    printf("%sDATA\n", prefix);

    for (int r = 0; r < st->range_count; r++) {
        int start = st->ranges[r].start_slot;
        int count = st->ranges[r].count;
        printf("%s    { SAVE/CLEAR range: RDA%d through RDA%d (%d slots) }\n",
               prefix, start, start + count - 1, count);
    }

    printf("%s    dt =", prefix);

    bool first = true;
    int i = 0;
    while (i <= st->max_slot) {
        if (!st->slots[i].used) { i++; continue; }

        if (!first) printf(",");
        first = false;

        if (st->slots[i].is_array && st->slots[i].max_index > 0) {
            int dim = st->slots[i].max_index;
            printf("\n%s        RDA%d(%d)", prefix, i, dim);
            if (!st->slots[i].bounds_certain)
                printf("  { inferred dimension }");
            i += dim;
        } else {
            printf("\n%s        RDA%d", prefix, i);
            if (st->slots[i].is_array)
                printf("  { indexed access observed }");
            i++;
        }
    }

    printf(";\n\n");
    symbol_table_free(st);
}

/* ── Preamble output ────────────────────────────────────────────────── */

static bool program_uses_gev(Program *prog) {
    for (Instruction *i = prog->instructions; i; i = i->next)
        for (int j = 0; j < i->operand_count; j++)
            if (i->operands[j].kind == OP_GEV) return true;
    return false;
}

static void print_preamble(Program *prog, bool raw, bool emit_data, const char *prefix,
                           GEVTable *gev, const char *input_file) {
    emit_source_header(stdout, prog, input_file, prefix);

    if (raw) {
        printf("{ Code start: 0x%04X }\n", prog->code_start);
    } else {
        printf("\n%sPROGRAM %s;\n\n", prefix, prog->program_name);
        if (emit_data) {
            if (gev && program_uses_gev(prog))
                printf("%sCOPY XXCGTSYS;\n\n", prefix);
            print_data_section(prog, prefix);
            printf("\n");
        }
    }
}

/* ── Main disassembly loop ──────────────────────────────────────────── */

static void print_disasm(Program *prog, bool raw, bool tabular, bool emit_data,
                         GEVTable *gev, const char *input_file) {
    const char *prefix = tabular ? TAB_PREFIX : "";

    TargetList targets = {0};
    if (!raw)
        collect_targets(prog, &targets);

    print_preamble(prog, raw, emit_data, prefix, gev, input_file);

    StringBuf sb;
    sbuf_init(&sb);

    FormatCtx ctx = {
        .sb = &sb,
        .targets = &targets,
        .gev = gev,
        .raw = raw,
    };

    bool in_proc = false;

    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        Target *tgt = raw ? NULL : target_find(&targets, instr->address);

        /* Procedure boundary */
        if (!raw && tgt && tgt->is_proc) {
            if (in_proc)
                printf("%sEND_PROC\n\n", prefix);
            printf("%sPROC %s =\n", prefix, tgt->name);
            in_proc = true;
        }

        /* Label */
        if (!raw && tgt && !tgt->is_proc)
            printf("%s  %s:\n", prefix, tgt->name);

        /* Skip bare RETURN at proc end in structured mode (not source-level).
         * Keep in tabular mode so the 0x48 byte is visible alongside hex. */
        if (!raw && !tabular && instr->mnemonic == MNEM_RETURN && instr->operand_count == 0) {
            Instruction *next = instr->next;
            bool at_proc_end = !next || (target_find(&targets, next->address) &&
                                         target_find(&targets, next->address)->is_proc);
            if (at_proc_end) continue;
        }

        /* Format and output the instruction */
        sbuf_reset(&sb);

        if (raw) {
            printf("%04X: ", instr->address);
            format_instruction(&ctx, instr);
            printf("%s\n", sb.data ? sb.data : "");
        } else {
            format_instruction(&ctx, instr);
            const char *text = sb.data ? sb.data : "";
            if (tabular)
                print_tabular_instruction(prog, instr, text);
            else
                print_structured_instruction(text);
        }
    }

    if (!raw && in_proc)
        printf("%sEND_PROC\n\n", prefix);

    sbuf_free(&sb);
    targets_free(&targets);
}

/* ── Entry point ────────────────────────────────────────────────────── */

int main(int argc, char **argv) {
#ifdef _WIN32
    /* Emit raw LF so output matches reference files (which are LF) and
     * does not pick up Windows stdout CRLF translation. */
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    const char *input_file = NULL;
    bool raw = false;
    bool tabular = false;
    bool emit_data = false;
    const char *include_paths[32];
    int include_path_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--raw") == 0) {
            raw = true;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tabular") == 0) {
            tabular = true;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--data") == 0) {
            emit_data = true;
        } else if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            if (include_path_count < 32)
                include_paths[include_path_count++] = argv[++i];
        } else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
            if (include_path_count < 32)
                include_paths[include_path_count++] = argv[i] + 2;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!input_file) {
        fprintf(stderr, "Error: no input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    Program *prog = cod_file_load(input_file);
    if (!prog) return 1;

    if (decode_program(prog) != 0) {
        fprintf(stderr, "Error: failed to decode program\n");
        program_free(prog);
        return 1;
    }

    GEVTable *gev = NULL;
    if (include_path_count > 0) {
        gev = gev_table_new();
        if (!gev_table_load(gev, include_paths, include_path_count)) {
            gev_table_free(gev);
            gev = NULL;
        }
    }

    print_disasm(prog, raw, tabular, emit_data, gev, input_file);

    if (gev) gev_table_free(gev);
    program_free(prog);
    return 0;
}
