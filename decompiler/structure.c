/*
 * structure.c - Control flow pattern matching for decompilation
 *
 * Walks the bytecode instruction stream and recognizes compiler patterns
 * to reconstruct structured TBOL source.
 */

#include "structure.h"
#include "operand_fmt.h"
#include "bytecode/opcodes.h"
#include "memfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -- Mode table ------------------------------------------------------- */

ModeTable *mode_table_new(void) { return calloc(1, sizeof(ModeTable)); }

void mode_table_free(ModeTable *mt) {
    if (!mt) return;
    free(mt->addrs);
    free(mt->modes);
    free(mt);
}

void mode_table_set(ModeTable *mt, uint16_t addr, PatternMode mode) {
    for (int i = 0; i < mt->count; i++) {
        if (mt->addrs[i] == addr) { mt->modes[i] = mode; return; }
    }
    if (mt->count >= mt->capacity) {
        mt->capacity = mt->capacity ? mt->capacity * 2 : 16;
        mt->addrs = realloc(mt->addrs, mt->capacity * sizeof(uint16_t));
        mt->modes = realloc(mt->modes, mt->capacity * sizeof(PatternMode));
    }
    mt->addrs[mt->count] = addr;
    mt->modes[mt->count] = mode;
    mt->count++;
}

PatternMode mode_table_get(ModeTable *mt, uint16_t addr) {
    if (!mt) return PMODE_FULL;
    for (int i = 0; i < mt->count; i++)
        if (mt->addrs[i] == addr) return mt->modes[i];
    return PMODE_FULL;
}

/* -- Instruction lookup helpers ---------------------------------------- */

static Instruction *instr_at(Program *prog, uint16_t addr) {
    for (Instruction *i = prog->instructions; i; i = i->next) {
        if (i->address == addr) return i;
    }
    return NULL;
}

static Instruction *next_instr(Instruction *i) {
    return i ? i->next : NULL;
}

static uint16_t after_addr(Instruction *i) {
    return i ? (uint16_t)(i->address + i->length) : 0;
}

static bool is_conditional(Mnemonic m) {
    return m >= MNEM_CJEQ && m <= MNEM_CJGE;
}

/* -- Label tracking ---------------------------------------------------- */

typedef struct {
    uint16_t *addrs;
    bool *emitted;
    int count;
    int capacity;
} LabelSet;

static void ls_add(LabelSet *ls, uint16_t addr) {
    for (int i = 0; i < ls->count; i++)
        if (ls->addrs[i] == addr) return;
    if (ls->count >= ls->capacity) {
        ls->capacity = ls->capacity ? ls->capacity * 2 : 64;
        ls->addrs = realloc(ls->addrs, ls->capacity * sizeof(uint16_t));
        ls->emitted = realloc(ls->emitted, ls->capacity * sizeof(bool));
    }
    ls->addrs[ls->count] = addr;
    ls->emitted[ls->count] = false;
    ls->count++;
}

static bool ls_contains(LabelSet *ls, uint16_t addr) {
    for (int i = 0; i < ls->count; i++)
        if (ls->addrs[i] == addr) return true;
    return false;
}

/* Check if label needs emission (exists and not yet emitted). Marks as emitted. */
static bool ls_needs_emit(LabelSet *ls, uint16_t addr) {
    for (int i = 0; i < ls->count; i++) {
        if (ls->addrs[i] == addr) {
            if (ls->emitted[i]) return false;
            ls->emitted[i] = true;
            return true;
        }
    }
    return false;
}

/* -- Proc/label name resolution ---------------------------------------- */

static const char *resolve_name(uint16_t addr, ProcList *procs, char *buf, int bufsize) {
    for (int i = 0; i < procs->count; i++) {
        if (procs->procs[i].start_addr == addr) {
            if (procs->procs[i].is_main) return "main";
            snprintf(buf, bufsize, "proc_%d", procs->procs[i].proc_num);
            return buf;
        }
    }
    snprintf(buf, bufsize, "label_%d", addr);
    return buf;
}

/* -- XXCGTSYS symbol resolution --------------------------------------- */

/* SET_FUNCTION action codes */
static const char *resolve_action(int val) {
    switch (val) {
        case 0:  return "NORMAL";
        case 16: return "DISABLE";
        case 32: return "FILTER";
        case 64: return "FILTER_ON";
        default: return NULL;
    }
}

/* Function key IDs (TRIGGER_FUNCTION, SET_FUNCTION operand 0) */
static const char *function_key_names[] = {
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
#define FUNCTION_KEY_COUNT ((int)(sizeof(function_key_names)/sizeof(function_key_names[0])))

static const char *resolve_function_key(int val) {
    if (val >= 0 && val < FUNCTION_KEY_COUNT && function_key_names[val])
        return function_key_names[val];
    return NULL;
}

/* SYS_RETURN_CODE values (from XXCGTSYS RET_* DEFINEs) */
static const char *return_code_names[] = {
    [0]  = "RET_OK",
    [1]  = "RET_HARDWARE_ERROR",
    [2]  = "RET_TIMEOUT",
    [3]  = "RET_BEYOND_EOF",
    [4]  = "RET_PRINTER_OUT_OF_PAPER",
    [5]  = "RET_OVERFLOW",
    [6]  = "RET_UNDERFLOW",
    [7]  = "RET_ZERO",
    [8]  = "RET_NOT_ZERO",
    [9]  = "RET_ALL_ONES",
    [10] = "RET_SOME_ONES",
    [11] = "RET_NO_ONES",
    /* 12 is not defined */
    [13] = "RET_NOT_OPEN",
    [14] = "RET_NOT_FOUND",
    [15] = "RET_DISK_FULL",
    [16] = "RET_BAD_LENGTH",
    [17] = "RET_BAD_POINTER",
    [18] = "RET_END_OF_FILE",
    [19] = "RET_STACK_EMPTY",
    [20] = "RET_STACK_FULL",
    [21] = "RET_NOT_ARRIVED",
    [22] = "RET_ALPHA",
    [23] = "RET_INTEGER",
    [24] = "RET_DECIMAL",
    [25] = "RET_DIVIDE_BY_ZERO",
    [26] = "RET_ALREADY_OPEN",
    [27] = "RET_NOT_ENOUGH_SLOTS",
};
#define RETURN_CODE_COUNT ((int)(sizeof(return_code_names)/sizeof(return_code_names[0])))

static const char *resolve_return_code(int val) {
    if (val >= 0 && val < RETURN_CODE_COUNT && return_code_names[val])
        return return_code_names[val];
    return NULL;
}

static int operand_int_val(Operand *op) {
    if (op->kind == OP_LITERAL_NUM) return op->value;
    if (op->kind == OP_LITERAL_STR && op->str_value) return atoi(op->str_value);
    return -1;
}

/* -- Indentation helper ------------------------------------------------ */

static void emit_indent(FILE *out, int level) {
    for (int i = 0; i < level; i++) fprintf(out, "  ");
}

/* -- Condition formatting ---------------------------------------------- */

static const char *inverted_op(Mnemonic m) {
    switch (m) {
        case MNEM_CJEQ: return "<>";
        case MNEM_CJNE: return "=";
        case MNEM_CJLT: return ">=";
        case MNEM_CJGT: return "<=";
        case MNEM_CJLE: return ">";
        case MNEM_CJGE: return "<";
        default: return "?";
    }
}

static const char *direct_op(Mnemonic m) {
    switch (m) {
        case MNEM_CJEQ: return "=";
        case MNEM_CJNE: return "<>";
        case MNEM_CJLT: return "<";
        case MNEM_CJGT: return ">";
        case MNEM_CJLE: return "<=";
        case MNEM_CJGE: return ">=";
        default: return "?";
    }
}

static void fmt_atom(FILE *out, Instruction *cj, GEVTable *gev, DefineTable *dt, bool invert) {
    char left[256], right[256];
    fmt_operand(left, sizeof(left), &cj->operands[0], gev, dt);
    fmt_operand(right, sizeof(right), &cj->operands[1], gev, dt);

    /* Resolve SYS_RETURN_CODE comparisons to RET_* symbolic names.
     * SYS_RETURN_CODE is GEV #1. When compared against a numeric literal
     * that matches a known return code, substitute the RET_* name. */
    if (cj->operand_count >= 2) {
        Operand *op0 = &cj->operands[0];
        Operand *op1 = &cj->operands[1];
        bool left_is_rc = (op0->kind == OP_GEV && op0->value == 1);
        bool right_is_rc = (op1->kind == OP_GEV && op1->value == 1);
        if (left_is_rc) {
            int val = operand_int_val(op1);
            const char *name = resolve_return_code(val);
            if (name) snprintf(right, sizeof(right), "%s", name);
        } else if (right_is_rc) {
            int val = operand_int_val(op0);
            const char *name = resolve_return_code(val);
            if (name) snprintf(left, sizeof(left), "%s", name);
        }
    }

    fprintf(out, "%s %s %s", left, invert ? inverted_op(cj->mnemonic) : direct_op(cj->mnemonic), right);
}

static Instruction *emit_condition(FILE *out, Instruction *instr, Program *prog,
                                    GEVTable *gev, DefineTable *dt,
                                    LabelSet *labels,
                                    uint16_t *fail_addr, uint16_t *body_addr) {
    if (!instr || !is_conditional(instr->mnemonic)) return instr;

    Instruction *next = next_instr(instr);

    /* Check for OR: CJxx -> check2; JUMP -> body; check2: ... */
    if (next && next->mnemonic == MNEM_JUMP &&
        !ls_contains(labels, next->address)) {
        uint16_t or_body = next->jump_target;
        uint16_t next_check = instr->jump_target;

        Instruction *first_or = instr_at(prog, next_check);
        if (!first_or || !is_conditional(first_or->mnemonic) ||
            ls_contains(labels, first_or->address)) {
            /* Can't form OR - the target is a label */
            goto simple;
        }

        fprintf(out, "(");
        fmt_atom(out, instr, gev, dt, true);

        Instruction *cur = first_or;

        while (cur && is_conditional(cur->mnemonic)) {
            Instruction *cur_next = next_instr(cur);
            if (cur_next && cur_next->mnemonic == MNEM_JUMP &&
                cur_next->jump_target == or_body &&
                !ls_contains(labels, cur_next->address)) {
                fprintf(out, " OR ");
                fmt_atom(out, cur, gev, dt, true);
                Instruction *nxt = instr_at(prog, cur->jump_target);
                if (nxt && ls_contains(labels, nxt->address)) {
                    /* Next OR term is a label target - stop here */
                    *fail_addr = cur->jump_target;
                    *body_addr = or_body;
                    fprintf(out, ")");
                    return next_instr(cur);
                }
                cur = nxt;
            } else {
                fprintf(out, " OR ");
                fmt_atom(out, cur, gev, dt, true);
                *fail_addr = cur->jump_target;
                *body_addr = or_body;
                fprintf(out, ")");

                Instruction *after_or = instr_at(prog, or_body);
                if (after_or && is_conditional(after_or->mnemonic) &&
                    after_or->jump_target == *fail_addr &&
                    !ls_contains(labels, after_or->address)) {
                    fprintf(out, " AND ");
                    Instruction *and_cur = after_or;
                    while (and_cur && is_conditional(and_cur->mnemonic) &&
                           and_cur->jump_target == *fail_addr) {
                        Instruction *and_next = next_instr(and_cur);
                        if (and_next && is_conditional(and_next->mnemonic) &&
                            and_next->jump_target == *fail_addr &&
                            !ls_contains(labels, and_next->address)) {
                            fmt_atom(out, and_cur, gev, dt, true);
                            fprintf(out, " AND ");
                            and_cur = and_next;
                        } else {
                            fmt_atom(out, and_cur, gev, dt, true);
                            *body_addr = after_addr(and_cur);
                            return next_instr(and_cur);
                        }
                    }
                }
                return next_instr(cur);
            }
        }
        fprintf(out, ")");
        *fail_addr = instr->jump_target;
        *body_addr = after_addr(next);
        return instr_at(prog, next_check);
    }

    /* Check for AND: consecutive CJxx to same target */
    if (next && is_conditional(next->mnemonic) &&
        next->jump_target == instr->jump_target &&
        !ls_contains(labels, next->address)) {
        fmt_atom(out, instr, gev, dt, true);
        Instruction *cur = next;
        while (cur && is_conditional(cur->mnemonic) &&
               cur->jump_target == instr->jump_target) {
            fprintf(out, " AND ");
            fmt_atom(out, cur, gev, dt, true);
            Instruction *cn = next_instr(cur);
            if (!cn || !is_conditional(cn->mnemonic) ||
                cn->jump_target != instr->jump_target ||
                ls_contains(labels, cn->address)) {
                *fail_addr = instr->jump_target;
                *body_addr = after_addr(cur);
                return next_instr(cur);
            }
            cur = cn;
        }
        *fail_addr = instr->jump_target;
        *body_addr = after_addr(cur);
        return next_instr(cur);
    }

simple:
    /* Simple condition */
    fmt_atom(out, instr, gev, dt, true);
    *fail_addr = instr->jump_target;
    *body_addr = after_addr(instr);
    return next_instr(instr);
}

/* -- Statement emission ------------------------------------------------ */

static void emit_verb(FILE *out, Instruction *instr, ProcList *procs,
                       GEVTable *gev, DefineTable *dt, StructMap *sm,
                       bool uses_xxcgtsys, int ind) {
    char buf[256], target_buf[32];

    if (instr->mnemonic == MNEM_BREAK) return;

    /* CLEAR/SAVE with struct name - emit struct name instead of slot */
    if ((instr->mnemonic == MNEM_CLEAR || instr->mnemonic == MNEM_SAVE) &&
        instr->var_count > 1 && instr->operand_count >= 1 &&
        instr->operands[0].kind == OP_RDA && !instr->operands[0].indexed) {
        const char *sname = struct_map_lookup(sm, instr->operands[0].value);
        if (sname) {
            emit_indent(out, ind);
            fprintf(out, "%s %s", instr->mnem_str, sname);
            for (int i = 1; i < instr->operand_count; i++) {
                fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
                fprintf(out, ", %s", buf);
            }
            fprintf(out, ";\n");
            return;
        }
    }

    /* JUMP -> GOTO */
    if (instr->mnemonic == MNEM_JUMP) {
        emit_indent(out, ind);
        fprintf(out, "GOTO %s;\n",
                resolve_name(instr->jump_target, procs, target_buf, sizeof(target_buf)));
        return;
    }

    /* CALL -> proc_name [args]; */
    if (instr->mnemonic == MNEM_CALL && instr->has_jump) {
        emit_indent(out, ind);
        /* CALL with jump offset 0 = unresolved (undefined proc) */
        if (instr->jump_offset == 0)
            fprintf(out, "_undefined");
        else
            fprintf(out, "%s", resolve_name(instr->jump_target, procs, target_buf, sizeof(target_buf)));
        for (int i = 0; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, "%s%s", i > 0 ? ", " : " ", buf);
        }
        fprintf(out, ";\n");
        return;
    }

    /* MOVE_ABS */
    if (instr->mnemonic == MNEM_MOVE_ABS) {
        emit_indent(out, ind);
        fprintf(out, "MOVE");
        for (int i = 0; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, "%s%s", i > 0 ? ", " : " ", buf);
        }
        fprintf(out, ", ABS;\n");
        return;
    }

    /* SEND with modifiers */
    if (instr->mnemonic == MNEM_SEND) {
        emit_indent(out, ind);
        fprintf(out, "SEND");
        for (int i = 0; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, "%s%s", i > 0 ? ", " : " ", buf);
        }
        if (instr->send_timeout) fprintf(out, ", TIMEOUT(%d)", instr->send_timeout);
        if (instr->send_flags & 0x04) fprintf(out, ", PRIORITY");
        if (instr->send_flags & 0x02) fprintf(out, ", OPT_HDRS");
        fprintf(out, ";\n");
        return;
    }

    /* RETURN/EXIT with argument */
    if ((instr->mnemonic == MNEM_RETURN || instr->mnemonic == MNEM_EXIT) &&
        instr->operand_count > 0) {
        emit_indent(out, ind);
        fprintf(out, "%s", instr->mnem_str);
        for (int i = 0; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, "%s%s", i > 0 ? ", " : " ", buf);
        }
        fprintf(out, ";\n");
        return;
    }

    /* TRIGGER_FUNCTION - resolve function key ID to symbolic name */
    if (instr->opcode == OP_TRIG_FUNC && uses_xxcgtsys && instr->operand_count >= 1) {
        int func_val = operand_int_val(&instr->operands[0]);
        const char *func_name = resolve_function_key(func_val);
        if (func_name) {
            emit_indent(out, ind);
            fprintf(out, "TRIGGER_FUNCTION %s;\n", func_name);
            return;
        }
    }

    /* SET_ATTRIBUTE - reconstruct attribute string from 4 raw bytes.
     * Attribute keywords are separated by SPACE, not comma: TBOL.EXE's
     * parser treats comma as a terminator and silently drops everything
     * after the first keyword (leaving 80 80 padding in the encoded
     * bytes for color). Observed in original disassemblies:
     *   'DISPLAY COLOR(0,2)'  'ACTION COLOR(0,4)'
     * The comma inside COLOR(fg,bg) is part of that sub-expression and
     * is fine. */
    if (instr->mnemonic == MNEM_SET_ATTRIBUTE && instr->operand_count >= 5) {
        emit_indent(out, ind);
        fmt_operand(buf, sizeof(buf), &instr->operands[0], gev, dt);
        fprintf(out, "SET_ATTRIBUTE %s, '", buf);

        uint8_t state = (uint8_t)instr->operands[1].value;
        uint8_t form  = (uint8_t)instr->operands[2].value;
        uint8_t fg    = (uint8_t)instr->operands[3].value;
        uint8_t bg    = (uint8_t)instr->operands[4].value;

        bool need_sep = false;
        switch (state & 0xF0) {
            case 0x80: fprintf(out, "ACTION"); need_sep = true; break;
            case 0x40: fprintf(out, "DISPLAY"); need_sep = true; break;
            case 0x20: fprintf(out, "INPUT"); need_sep = true; break;
            default: break;
        }
        if (state & 0x08) {
            if (need_sep) fprintf(out, " ");
            fprintf(out, "PP_NOT_FIRED"); need_sep = true;
        }
        if (form & 0x80) {
            if (need_sep) fprintf(out, " ");
            fprintf(out, "ALPHABETIC"); need_sep = true;
        }
        if (form & 0x40) {
            if (need_sep) fprintf(out, " ");
            fprintf(out, "NUMERIC"); need_sep = true;
        }
        if (form & 0x10) {
            if (need_sep) fprintf(out, " ");
            fprintf(out, "PASSWORD"); need_sep = true;
        }
        if (form & 0x20) {
            if (need_sep) fprintf(out, " ");
            if (instr->operand_count > 5 && instr->operands[5].str_value)
                fprintf(out, "FORM(%s)", instr->operands[5].str_value);
            else
                fprintf(out, "FORM()");
            need_sep = true;
        }
        if (fg != 0x80 || bg != 0x80) {
            if (need_sep) fprintf(out, " ");
            fprintf(out, "COLOR(%d,%d)", fg, bg);
        }
        fprintf(out, "';\n");
        return;
    }

    /* SET_FUNCTION - resolve action code, use DEFINE identifiers for blobs */
    if (instr->mnemonic == MNEM_SET_FUNCTION && instr->operand_count >= 2) {
        emit_indent(out, ind);
        fprintf(out, "SET_FUNCTION ");

        /* Operand 0: function key ID - resolve to symbolic name */
        Operand *key_op = &instr->operands[0];
        int key_val = operand_int_val(key_op);
        const char *key_name = (uses_xxcgtsys) ? resolve_function_key(key_val) : NULL;
        if (key_name) {
            fprintf(out, "%s, ", key_name);
        } else {
            fmt_operand(buf, sizeof(buf), key_op, gev, dt);
            fprintf(out, "%s, ", buf);
        }

        /* Operand 1: action - resolve to XXCGTSYS name if available.
         * Only resolve if the operand is truly numeric (not a string like 'display'). */
        Operand *action_op = &instr->operands[1];
        bool is_numeric_action = (action_op->kind == OP_LITERAL_NUM) ||
            (action_op->kind == OP_LITERAL_STR && action_op->str_value &&
             action_op->str_value[0] >= '0' && action_op->str_value[0] <= '9');
        int action_val = operand_int_val(action_op);
        const char *action_name = (uses_xxcgtsys && is_numeric_action)
            ? resolve_action(action_val) : NULL;
        if (action_name) {
            fprintf(out, "%s", action_name);
        } else {
            fmt_operand(buf, sizeof(buf), &instr->operands[1], gev, dt);
            fprintf(out, "%s", buf);
        }

        /* Remaining operands (program name, params) */
        for (int i = 2; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, ", %s", buf);
        }
        fprintf(out, ";\n");
        return;
    }

    /* GOTO_DEPENDING_ON */
    /* MAKE_FORMAT - colon syntax for format specs */
    if (instr->mnemonic == MNEM_MAKE_FORMAT && instr->operand_count >= 4) {
        emit_indent(out, ind);
        fmt_operand(buf, sizeof(buf), &instr->operands[0], gev, dt);
        fprintf(out, "MAKE_FORMAT %s,\n", buf);

        for (int i = 1; i + 2 < instr->operand_count; i += 3) {
            emit_indent(out, ind + 1);
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, "%s", buf);

            int fix_val = operand_int_val(&instr->operands[i + 1]);
            int emb_val = operand_int_val(&instr->operands[i + 2]);
            if (fix_val > 0 && emb_val > 0) fprintf(out, ":%d:%d", fix_val, emb_val);
            else if (fix_val > 0) fprintf(out, ":%d", fix_val);
            else if (emb_val > 0) fprintf(out, "::%d", emb_val);

            if (i + 5 < instr->operand_count) fprintf(out, ",");
            fprintf(out, "\n");
        }
        emit_indent(out, ind + 1);
        fprintf(out, ";\n");
        return;
    }

    if (instr->mnemonic == MNEM_GOTO_DEPENDING_ON) {
        emit_indent(out, ind);
        fprintf(out, "GOTO_DEPENDING_ON");
        if (instr->operand_count > 0) {
            fmt_operand(buf, sizeof(buf), &instr->operands[0], gev, dt);
            fprintf(out, " %s", buf);
        }
        for (int i = 1; i < instr->operand_count; i++) {
            Operand *op = &instr->operands[i];
            uint16_t target = (op->kind == OP_LITERAL_STR && op->str_value)
                ? (uint16_t)atoi(op->str_value) : (uint16_t)op->value;
            fprintf(out, ", %s", resolve_name(target, procs, target_buf, sizeof(target_buf)));
        }
        fprintf(out, ";\n");
        return;
    }

    /* MOVE_BLOCK -> MOVE (array-to-array) */
    if (instr->opcode == OP_MOVE_BLOCK && instr->operand_count >= 2) {
        emit_indent(out, ind);
        fmt_operand(buf, sizeof(buf), &instr->operands[0], gev, dt);
        fprintf(out, "MOVE %s, ", buf);
        fmt_operand(buf, sizeof(buf), &instr->operands[1], gev, dt);
        fprintf(out, "%s;\n", buf);
        return;
    }

    /* DEFINE_FIELD - bytecode omits row, insert '0' as placeholder.
     * Bytecode operands: name, col, width, height, object_id [, state]
     * Source operands:   name, row, col, width, height, object_id [, state] */
    if (instr->mnemonic == MNEM_DEFINE_FIELD && instr->operand_count >= 5) {
        emit_indent(out, ind);
        fmt_operand(buf, sizeof(buf), &instr->operands[0], gev, dt);
        fprintf(out, "DEFINE_FIELD %s, '0'", buf);
        for (int i = 1; i < instr->operand_count; i++) {
            fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
            fprintf(out, ", %s", buf);
        }
        fprintf(out, ";\n");
        return;
    }

    /* Generic verb */
    emit_indent(out, ind);
    fprintf(out, "%s", instr->mnem_str);
    for (int i = 0; i < instr->operand_count; i++) {
        fmt_operand(buf, sizeof(buf), &instr->operands[i], gev, dt);
        fprintf(out, "%s%s", i > 0 ? ", " : " ", buf);
    }
    if (instr->has_jump && instr->mnemonic != MNEM_JUMP) {
        fprintf(out, "%s%s", instr->operand_count > 0 ? ", " : " ",
                resolve_name(instr->jump_target, procs, target_buf, sizeof(target_buf)));
    }
    fprintf(out, ";\n");
}

/* -- Structured emission ----------------------------------------------- */

static Instruction *emit_block(FILE *out, Instruction *start, uint16_t end_addr,
                                uint16_t proc_end_addr,
                                Program *prog, ProcList *procs, LabelSet *labels,
                                GEVTable *gev, DefineTable *dt, StructMap *sm,
                                ModeTable *mt, bool uses_xxcgtsys, int ind);

/*
 * Emit a body range as either a single inline statement or a DO block.
 * Handles labels inside the body correctly. Used after "IF cond THEN "
 * or "WHILE cond THEN " has already been written to `out`.
 */
static void emit_body(FILE *out, uint16_t body_addr, uint16_t body_end,
                       uint16_t proc_end_addr,
                       int body_count, Program *prog, ProcList *procs,
                       LabelSet *labels, GEVTable *gev, DefineTable *dt,
                       StructMap *sm, ModeTable *mt, bool uses_xxcgtsys, int ind) {
    if (body_count == 0) {
        fprintf(out, ";\n");
        return;
    }

    /* Check if any body instruction has a label - forces DO block */
    bool needs_block = (body_count > 1);
    if (!needs_block) {
        for (Instruction *t = instr_at(prog, body_addr);
             t && t->address < body_end; t = t->next) {
            if (ls_contains(labels, t->address)) { needs_block = true; break; }
        }
    }

    if (needs_block) {
        fprintf(out, "DO\n");
        emit_block(out, instr_at(prog, body_addr), body_end, proc_end_addr,
                   prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind + 1);
        emit_indent(out, ind);
        fprintf(out, "END;\n");
    } else {
        Instruction *body_instr = instr_at(prog, body_addr);
        emit_verb(out, body_instr, procs, gev, dt, sm, uses_xxcgtsys, 0);
    }
}

static Instruction *emit_block(FILE *out, Instruction *start, uint16_t end_addr,
                                uint16_t proc_end_addr,
                                Program *prog, ProcList *procs, LabelSet *labels,
                                GEVTable *gev, DefineTable *dt, StructMap *sm,
                                ModeTable *mt, bool uses_xxcgtsys, int ind) {
    Instruction *ip = start;
    char target_buf[32];

    while (ip && ip->address < end_addr) {
        if (ls_needs_emit(labels, ip->address)) {
            emit_indent(out, ind - 1);
            fprintf(out, "%s:\n", resolve_name(ip->address, procs, target_buf, sizeof(target_buf)));
        }

        /* Conditional branch - try to match structured pattern */
        if (is_conditional(ip->mnemonic)) {
            uint16_t fail_addr, body_addr;
            PatternMode mode = mode_table_get(mt, ip->address);

            /* PMODE_FLAT: emit as IF direct_cond THEN GOTO target */
            if (mode == PMODE_FLAT) {
                emit_indent(out, ind);
                fprintf(out, "IF ");
                fmt_atom(out, ip, gev, dt, false);
                fprintf(out, " THEN GOTO %s;\n",
                        resolve_name(ip->jump_target, procs, target_buf, sizeof(target_buf)));
                ip = next_instr(ip);
                continue;
            }

            /* IF cond THEN GOTO: CJ(!cond) -> skip; JUMP -> target; skip:
             * Only use this shortcut if the JUMP isn't itself a label target,
             * otherwise the label would be lost. */
            Instruction *next = next_instr(ip);
            if (next && next->mnemonic == MNEM_JUMP &&
                after_addr(next) == ip->jump_target &&
                !ls_contains(labels, next->address)) {
                emit_indent(out, ind);
                fprintf(out, "IF ");
                fmt_atom(out, ip, gev, dt, true);
                fprintf(out, " THEN GOTO %s;\n",
                        resolve_name(next->jump_target, procs, target_buf, sizeof(target_buf)));
                ip = instr_at(prog, ip->jump_target);
                continue;
            }

            uint16_t cj_addr = ip->address;

            char cond_buf[1024];
            FILE *cond_f = mem_fopen_fixed(cond_buf, sizeof(cond_buf));
            (void)emit_condition(cond_f, ip, prog, gev, dt, labels, &fail_addr, &body_addr);
            mem_fclose_fixed(cond_f, cond_buf, sizeof(cond_buf));

            Instruction *body_last = NULL;
            for (Instruction *t = instr_at(prog, body_addr); t && t->address < fail_addr; t = t->next)
                body_last = t;

            /* WHILE: body ends with JUMP back to condition */
            if (body_last && body_last->mnemonic == MNEM_JUMP &&
                body_last->jump_target == cj_addr) {
                uint16_t while_body_end = body_last->address;

                int body_count = 0;
                for (Instruction *t = instr_at(prog, body_addr);
                     t && t->address < while_body_end; t = t->next)
                    body_count++;

                emit_indent(out, ind);
                fprintf(out, "WHILE %s THEN ", cond_buf);
                emit_body(out, body_addr, while_body_end, proc_end_addr, body_count,
                          prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind);

                ip = instr_at(prog, fail_addr);
                continue;
            }

            /* IF/THEN/ELSE: body ends with JUMP -> end
             * Only valid if there's at least one then-body instruction
             * before the JUMP separator.
             * Skip this pattern if mode is NO_ELSE. */
            if (mode != PMODE_NO_ELSE &&
                body_last && body_last->mnemonic == MNEM_JUMP &&
                body_last->jump_target > fail_addr &&
                body_last->address > body_addr) {
                uint16_t then_body_end = body_last->address;
                uint16_t end_addr_local = body_last->jump_target;

                int then_count = 0;
                for (Instruction *t = instr_at(prog, body_addr);
                     t && t->address < then_body_end; t = t->next)
                    then_count++;

                emit_indent(out, ind);
                fprintf(out, "IF %s THEN ", cond_buf);
                emit_body(out, body_addr, then_body_end, proc_end_addr, then_count,
                          prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind);

                /* ELSE IF or ELSE */
                Instruction *else_start = instr_at(prog, fail_addr);
                if (else_start && is_conditional(else_start->mnemonic)) {
                    emit_indent(out, ind);
                    fprintf(out, "ELSE ");

                    int else_count = 0;
                    for (Instruction *t = else_start;
                         t && t->address < end_addr_local; t = t->next)
                        else_count++;

                    if (else_count == 1 && !is_conditional(else_start->mnemonic)) {
                        emit_verb(out, else_start, procs, gev, dt, sm, uses_xxcgtsys, 0);
                    } else {
                        fprintf(out, "DO\n");
                        emit_block(out, else_start, end_addr_local, proc_end_addr,
                                   prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind + 1);
                        emit_indent(out, ind);
                        fprintf(out, "END;\n");
                    }
                } else {
                    int else_count = 0;
                    for (Instruction *t = instr_at(prog, fail_addr);
                         t && t->address < end_addr_local; t = t->next)
                        else_count++;

                    if (else_count > 0) {
                        emit_indent(out, ind);
                        if (else_count == 1) {
                            fprintf(out, "ELSE ");
                            emit_verb(out, instr_at(prog, fail_addr), procs, gev, dt, sm, uses_xxcgtsys, 0);
                        } else {
                            fprintf(out, "ELSE DO\n");
                            emit_block(out, instr_at(prog, fail_addr), end_addr_local, proc_end_addr,
                                       prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind + 1);
                            emit_indent(out, ind);
                            fprintf(out, "END;\n");
                        }
                    }
                }

                ip = instr_at(prog, end_addr_local);
                continue;
            }

            /* Simple IF/THEN */
            {
                int body_count = 0;
                for (Instruction *t = instr_at(prog, body_addr);
                     t && t->address < fail_addr; t = t->next)
                    body_count++;

                emit_indent(out, ind);
                fprintf(out, "IF %s THEN ", cond_buf);
                emit_body(out, body_addr, fail_addr, proc_end_addr, body_count,
                          prog, procs, labels, gev, dt, sm, mt, uses_xxcgtsys, ind);

                ip = instr_at(prog, fail_addr);
                continue;
            }
        }

        /* Unconditional JUMP -> GOTO */
        if (ip->mnemonic == MNEM_JUMP) {
            emit_indent(out, ind);
            fprintf(out, "GOTO %s;\n",
                    resolve_name(ip->jump_target, procs, target_buf, sizeof(target_buf)));
            ip = next_instr(ip);
            continue;
        }

        /* Bare RETURN at proc end - skip unless the next address is a label target.
         * Only suppress at the true proc end (next instruction is at or past
         * proc_end_addr), not at DO block boundaries. */
        if (ip->mnemonic == MNEM_RETURN && ip->operand_count == 0) {
            Instruction *next = next_instr(ip);
            if ((!next || next->address >= proc_end_addr) &&
                !ls_contains(labels, after_addr(ip))) {
                ip = next;
                continue;
            }
        }

        /* Regular instruction */
        emit_verb(out, ip, procs, gev, dt, sm, uses_xxcgtsys, ind);
        ip = next_instr(ip);
    }

    return ip;
}

/* -- Public API -------------------------------------------------------- */

void emit_structured_proc(FILE *out, Program *prog, ProcBoundary *pb,
                           ProcList *procs, GEVTable *gev, DefineTable *dt,
                           StructMap *sm, ModeTable *mt, int ind) {
    /* Determine if XXCGTSYS symbols are available (for SET_FUNCTION action resolution) */
    bool uses_xxcgtsys = (gev != NULL);

    /* Collect labels needed within this procedure */
    LabelSet labels = {0};
    for (Instruction *i = prog->instructions; i; i = i->next) {
        if (i->address < pb->start_addr || i->address >= pb->end_addr) continue;

        if (i->has_jump && !is_conditional(i->mnemonic) && i->mnemonic != MNEM_CALL) {
            if (i->jump_target >= pb->start_addr && i->jump_target < pb->end_addr)
                ls_add(&labels, i->jump_target);
        }
        if (i->mnemonic == MNEM_GOTO_DEPENDING_ON) {
            for (int j = 1; j < i->operand_count; j++) {
                Operand *op = &i->operands[j];
                uint16_t target = (op->kind == OP_LITERAL_STR && op->str_value)
                    ? (uint16_t)atoi(op->str_value) : (uint16_t)op->value;
                if (target >= pb->start_addr && target < pb->end_addr)
                    ls_add(&labels, target);
            }
        }
        if (is_conditional(i->mnemonic)) {
            if (i->jump_target >= pb->start_addr && i->jump_target < pb->end_addr)
                ls_add(&labels, i->jump_target);
        }
    }

    emit_block(out, instr_at(prog, pb->start_addr), pb->end_addr, pb->end_addr,
               prog, procs, &labels, gev, dt, sm, mt, uses_xxcgtsys, ind);

    free(labels.addrs);
    free(labels.emitted);
}
