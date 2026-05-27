/*
 * operand_fmt.c — Operand formatting for source emission
 *
 * Converts decoded IR operands to TBOL source text.
 * Must produce text that the compiler will parse back to identical bytecode.
 */

#include "operand_fmt.h"
#include "bytecode/opcodes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Define table ───────────────────────────────────────────────────── */

DefineTable *define_table_new(void) {
    DefineTable *dt = calloc(1, sizeof(DefineTable));
    return dt;
}

void define_table_free(DefineTable *dt) {
    if (!dt) return;
    for (int i = 0; i < dt->count; i++) {
        free(dt->entries[i].hex_value);
        free(dt->entries[i].ident);
        free(dt->entries[i].raw_bytes);
    }
    free(dt->entries);
    free(dt);
}

/* Check if a byte sequence is all printable ASCII */
static bool is_printable(const char *s, int len) {
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

/*
 * Build an identifier from a byte string per the user's rules:
 * 1. Take longest run of printable non-whitespace chars that form a legal identifier
 * 2. If > 8 chars: first8 + "_" + rest
 * 3. If <= 8 chars: just the printable portion
 * 4. Lowercase
 */
static char *make_ident_from_bytes(const char *raw, int len) {
    /* Find longest run of printable non-whitespace chars */
    int best_start = 0, best_len = 0;
    int run_start = 0, run_len = 0;

    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)raw[i];
        if (c > 0x20 && c <= 0x7E && (isalnum(c) || c == '_')) {
            if (run_len == 0) run_start = i;
            run_len++;
        } else {
            if (run_len > best_len) {
                best_start = run_start;
                best_len = run_len;
            }
            run_len = 0;
        }
    }
    if (run_len > best_len) {
        best_start = run_start;
        best_len = run_len;
    }

    if (best_len == 0) {
        /* No printable chars — use hex prefix */
        char *ident = malloc(16);
        snprintf(ident, 16, "hex_%02x%02x",
                 (unsigned char)raw[0], len > 1 ? (unsigned char)raw[1] : 0);
        return ident;
    }

    char *ident;
    if (best_len > 8) {
        /* first8_rest */
        ident = malloc(best_len + 2); /* +1 for underscore, +1 for null */
        memcpy(ident, raw + best_start, 8);
        ident[8] = '_';
        memcpy(ident + 9, raw + best_start + 8, best_len - 8);
        ident[best_len + 1] = '\0';
    } else {
        ident = malloc(best_len + 1);
        memcpy(ident, raw + best_start, best_len);
        ident[best_len] = '\0';
    }

    /* Lowercase */
    for (char *p = ident; *p; p++)
        *p = tolower((unsigned char)*p);

    /* TBOL identifiers can't start with a digit. Rewrite the leading
     * digit to a visually-similar letter, mirroring the manual convention
     * used in encyclopedia (6100 → G100). Interior digits are unaffected. */
    if (ident[0] >= '0' && ident[0] <= '9') {
        static const char digit_letter[10] = {
            'o', 'i', 'z', 'e', 'a', 's', 'g', 't', 'b', 'p'
        };
        ident[0] = digit_letter[ident[0] - '0'];
    }

    return ident;
}

/* Build hex literal string from raw bytes */
static char *make_hex_literal(const char *raw, int len) {
    /* "0x" + 2 hex chars per byte + null */
    char *hex = malloc(2 + len * 2 + 1);
    hex[0] = '0';
    hex[1] = 'x';
    for (int i = 0; i < len; i++)
        snprintf(hex + 2 + i * 2, 3, "%02X", (unsigned char)raw[i]);
    hex[2 + len * 2] = '\0';
    return hex;
}

/* Check if this operand is already in the table */
static bool dt_has(DefineTable *dt, const char *raw, int len) {
    for (int i = 0; i < dt->count; i++) {
        if (dt->entries[i].raw_len == len &&
            memcmp(dt->entries[i].raw_bytes, raw, len) == 0)
            return true;
    }
    return false;
}

/* Ensure identifier uniqueness by appending _2, _3, etc. */
static char *make_unique_ident(DefineTable *dt, char *base) {
    /* Check if base is already used */
    bool conflict = false;
    for (int i = 0; i < dt->count; i++) {
        if (strcmp(dt->entries[i].ident, base) == 0) {
            conflict = true;
            break;
        }
    }
    if (!conflict) return base;

    /* Append suffix */
    for (int suffix = 2; suffix < 100; suffix++) {
        size_t cand_size = strlen(base) + 8;
        char *candidate = malloc(cand_size);
        snprintf(candidate, cand_size, "%s_%d", base, suffix);
        conflict = false;
        for (int i = 0; i < dt->count; i++) {
            if (strcmp(dt->entries[i].ident, candidate) == 0) {
                conflict = true;
                break;
            }
        }
        if (!conflict) {
            free(base);
            return candidate;
        }
        free(candidate);
    }
    return base;
}

static void dt_add(DefineTable *dt, const char *raw, int len) {
    if (dt_has(dt, raw, len)) return;

    if (dt->count >= dt->capacity) {
        dt->capacity = dt->capacity ? dt->capacity * 2 : 16;
        dt->entries = realloc(dt->entries, dt->capacity * sizeof(DefineEntry));
    }

    DefineEntry *e = &dt->entries[dt->count];
    e->raw_len = len;
    e->raw_bytes = malloc(len);
    memcpy(e->raw_bytes, raw, len);
    e->hex_value = make_hex_literal(raw, len);
    char *ident = make_ident_from_bytes(raw, len);
    e->ident = make_unique_ident(dt, ident);
    dt->count++;
}

static bool starts_with_ident_char(const char *s, int len) {
    if (len <= 0) return false;
    unsigned char c = (unsigned char)s[0];
    /* Accept digits here; make_ident_from_bytes rewrites the first char
     * to a visually-similar letter so the resulting identifier is valid
     * under both TBOL.EXE and tbolc. */
    return (c > 0x20 && c <= 0x7E && (isalnum(c) || c == '_'));
}

static void scan_operand_for_defines(DefineTable *dt, Operand *op) {
    if (!op) return;
    if (op->kind == OP_LITERAL_STR && op->str_value) {
        int slen = op->str_len > 0 ? op->str_len : (int)strlen(op->str_value);
        if (slen > 0 && !is_printable(op->str_value, slen) &&
            starts_with_ident_char(op->str_value, slen))
            dt_add(dt, op->str_value, slen);
    }
    if (op->indexed && op->index)
        scan_operand_for_defines(dt, op->index);
}

void define_table_scan(DefineTable *dt, Program *prog) {
    for (Instruction *instr = prog->instructions; instr; instr = instr->next) {
        for (int i = 0; i < instr->operand_count; i++)
            scan_operand_for_defines(dt, &instr->operands[i]);
    }
}

const char *define_table_lookup(DefineTable *dt, Operand *op) {
    if (!dt || !op || op->kind != OP_LITERAL_STR || !op->str_value) return NULL;
    int slen = op->str_len > 0 ? op->str_len : (int)strlen(op->str_value);
    for (int i = 0; i < dt->count; i++) {
        if (dt->entries[i].raw_len == slen &&
            memcmp(dt->entries[i].raw_bytes, op->str_value, slen) == 0)
            return dt->entries[i].ident;
    }
    return NULL;
}

/* ── Struct map ─────────────────────────────────────────────────────── */

StructMap *struct_map_new(void) {
    return calloc(1, sizeof(StructMap));
}

void struct_map_free(StructMap *sm) {
    if (!sm) return;
    free(sm->entries);
    free(sm);
}

void struct_map_add(StructMap *sm, int start_slot, int count, const char *name) {
    if (sm->count >= sm->capacity) {
        sm->capacity = sm->capacity ? sm->capacity * 2 : 8;
        sm->entries = realloc(sm->entries, sm->capacity * sizeof(StructEntry));
    }
    StructEntry *e = &sm->entries[sm->count++];
    e->start_slot = start_slot;
    e->count = count;
    snprintf(e->name, sizeof(e->name), "%s", name);
}

const char *struct_map_lookup(StructMap *sm, int slot) {
    if (!sm) return NULL;
    for (int i = 0; i < sm->count; i++)
        if (sm->entries[i].start_slot == slot)
            return sm->entries[i].name;
    return NULL;
}

/* ── Operand formatting ─────────────────────────────────────────────── */

int fmt_operand(char *buf, int bufsize, Operand *op, GEVTable *gev, DefineTable *dt) {
    if (!op || !buf) return 0;

    switch (op->kind) {
        case OP_REG_I:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                return snprintf(buf, bufsize, "I%d(%s)", op->value, idx_buf);
            }
            return snprintf(buf, bufsize, "I%d", op->value);
        case OP_REG_D:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                return snprintf(buf, bufsize, "D%d(%s)", op->value, idx_buf);
            }
            return snprintf(buf, bufsize, "D%d", op->value);
        case OP_REG_P:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                return snprintf(buf, bufsize, "P%d(%s)", op->value, idx_buf);
            }
            return snprintf(buf, bufsize, "P%d", op->value);
        case OP_RDA:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                return snprintf(buf, bufsize, "RDA%d(%s)", op->value, idx_buf);
            }
            return snprintf(buf, bufsize, "RDA%d", op->value);
        case OP_PEV:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                return snprintf(buf, bufsize, "&%d(%s)", op->value, idx_buf);
            }
            return snprintf(buf, bufsize, "&%d", op->value);
        case OP_GEV:
            if (op->indexed && op->index) {
                char idx_buf[64];
                fmt_operand(idx_buf, sizeof(idx_buf), op->index, gev, dt);
                if (gev) {
                    const char *name = gev_lookup(gev, op->value);
                    if (name) return snprintf(buf, bufsize, "%s(%s)", name, idx_buf);
                }
                return snprintf(buf, bufsize, "#%d(%s)", op->value, idx_buf);
            }
            if (gev) {
                const char *name = gev_lookup(gev, op->value);
                if (name) return snprintf(buf, bufsize, "%s", name);
            }
            return snprintf(buf, bufsize, "#%d", op->value);
        case OP_LITERAL_NUM: {
            /* Numeric literal — emit as quoted string (compiler expects this) */
            char num_buf[32];
            snprintf(num_buf, sizeof(num_buf), "%d", op->value);
            return snprintf(buf, bufsize, "'%s'", num_buf);
        }
        case OP_LITERAL_STR:
            if (op->str_value) {
                int slen = op->str_len > 0 ? op->str_len : (int)strlen(op->str_value);

                /* Check define table first for non-printable strings */
                if (dt) {
                    const char *ident = define_table_lookup(dt, op);
                    if (ident) return snprintf(buf, bufsize, "%s", ident);
                }

                if (is_printable(op->str_value, slen)) {
                    /* Escape single quotes (\') and backslashes (\\) */
                    int n = snprintf(buf, bufsize, "'");
                    for (int i = 0; i < slen && n < bufsize - 3; i++) {
                        if (op->str_value[i] == '\'')
                            n += snprintf(buf + n, bufsize - n, "\\'");
                        else if (op->str_value[i] == '\\')
                            n += snprintf(buf + n, bufsize - n, "\\\\");
                        else
                            n += snprintf(buf + n, bufsize - n, "%c", op->str_value[i]);
                    }
                    n += snprintf(buf + n, bufsize - n, "'");
                    return n;
                } else {
                    /* Fallback: quoted string with \xNN escapes */
                    int n = snprintf(buf, bufsize, "'");
                    for (int i = 0; i < slen && n < bufsize - 5; i++) {
                        unsigned char c = (unsigned char)op->str_value[i];
                        if (c >= 0x20 && c <= 0x7E && c != '\'') {
                            n += snprintf(buf + n, bufsize - n, "%c", c);
                        } else {
                            n += snprintf(buf + n, bufsize - n, "\\x%02X", c);
                        }
                    }
                    n += snprintf(buf + n, bufsize - n, "'");
                    return n;
                }
            }
            return snprintf(buf, bufsize, "''");
        default:
            return snprintf(buf, bufsize, "?");
    }
}

/* Invert CJxx mnemonic to get source-level comparison operator.
 * TBOL bytecode: CJxx means "jump if condition TRUE",
 * but for IF/THEN we want the body to execute, so we need
 * the non-inverted form for GOTO and inverted for IF body. */
static const char *cmp_op_str(Mnemonic m, bool invert) {
    if (invert) {
        switch (m) {
            case MNEM_CJEQ: return "<>";
            case MNEM_CJNE: return "=";
            case MNEM_CJLT: return ">=";
            case MNEM_CJGT: return "<=";
            case MNEM_CJLE: return ">";
            case MNEM_CJGE: return "<";
            default: return "?";
        }
    } else {
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
}

int fmt_condition(char *buf, int bufsize, Instruction *instr, GEVTable *gev) {
    if (!instr || instr->operand_count < 2) return 0;

    char left[128], right[128];
    fmt_operand(left, sizeof(left), &instr->operands[0], gev, NULL);
    fmt_operand(right, sizeof(right), &instr->operands[1], gev, NULL);

    /* Non-inverted: the condition as the bytecode tests it */
    const char *op = cmp_op_str(instr->mnemonic, false);
    return snprintf(buf, bufsize, "%s %s %s", left, op, right);
}

const char *fmt_mnemonic(Instruction *instr) {
    if (!instr) return "???";
    /* Map decoded mnemonic to source verb name */
    return instr->mnem_str;
}
