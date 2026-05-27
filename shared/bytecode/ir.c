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
#include "ir.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * Operand functions
 */
Operand *operand_new(OperandKind kind, int value) {
    Operand *op = calloc(1, sizeof(Operand));
    op->kind = kind;
    op->value = value;
    return op;
}

Operand *operand_new_str(const char *str, int len) {
    Operand *op = calloc(1, sizeof(Operand));
    op->kind = OP_LITERAL_STR;
    op->str_value = malloc(len + 1);
    memcpy(op->str_value, str, len);
    op->str_value[len] = '\0';
    op->str_len = len;
    return op;
}

Operand *operand_new_indexed(Operand *base, Operand *idx) {
    base->indexed = true;
    base->index = idx;
    return base;
}

/*
 * Free operand contents (but not the operand struct itself)
 */
static void operand_cleanup(Operand *op) {
    if (!op) return;
    if (op->str_value) {
        free(op->str_value);
        op->str_value = NULL;
    }
    if (op->index) {
        operand_cleanup(op->index);
        free(op->index);
        op->index = NULL;
    }
}

void operand_free(Operand *op) {
    if (!op) return;
    operand_cleanup(op);
    free(op);
}

char *operand_to_string(Operand *op) {
    char buf[256];

    switch (op->kind) {
        case OP_REG_I:
            snprintf(buf, sizeof(buf), "I%d", op->value);
            break;
        case OP_REG_D:
            snprintf(buf, sizeof(buf), "D%d", op->value);
            break;
        case OP_REG_P:
            snprintf(buf, sizeof(buf), "P%d", op->value);
            break;
        case OP_RDA:
            snprintf(buf, sizeof(buf), "RDA%d", op->value);
            break;
        case OP_PEV:
            snprintf(buf, sizeof(buf), "&%d", op->value);
            break;
        case OP_GEV:
            snprintf(buf, sizeof(buf), "#%d", op->value);
            break;
        case OP_LITERAL_NUM:
            if (op->is_hex) {
                snprintf(buf, sizeof(buf), "0x%X", op->value);
            } else {
                snprintf(buf, sizeof(buf), "%d", op->value);
            }
            break;
        case OP_LITERAL_STR:
            if (op->str_value && op->str_len > 0) {
                /* If ALL bytes are non-printable, emit as hex literal: 0xNNNN */
                bool all_nonprintable = true;
                for (int i = 0; i < op->str_len; i++) {
                    unsigned char c = (unsigned char)op->str_value[i];
                    if (c >= 0x20 && c <= 0x7E) { all_nonprintable = false; break; }
                }
                if (all_nonprintable) {
                    char *p = buf;
                    *p++ = '0';
                    *p++ = 'x';
                    for (int i = 0; i < op->str_len && p < buf + sizeof(buf) - 3; i++) {
                        p += snprintf(p, (size_t)(buf + sizeof(buf) - p), "%02x",
                                     (unsigned char)op->str_value[i]);
                    }
                    break;
                }

                char *p = buf;
                *p++ = '\'';
                for (int i = 0; i < op->str_len && p < buf + sizeof(buf) - 8; i++) {
                    unsigned char c = (unsigned char)op->str_value[i];
                    if (c >= 0x20 && c <= 0x7E && c != '\'' && c != '\\') {
                        /* Printable ASCII (except quote and backslash) */
                        *p++ = c;
                    } else if (c == '\'') {
                        /* Escaped single quote */
                        *p++ = '\\';
                        *p++ = '\'';
                    } else if (c == '\\') {
                        /* Escaped backslash */
                        *p++ = '\\';
                        *p++ = '\\';
                    } else {
                        /* Non-printable: emit as \xNN */
                        p += snprintf(p, (size_t)(buf + sizeof(buf) - p), "\\x%02x", c);
                    }
                }
                *p++ = '\'';
                *p = '\0';
            } else {
                snprintf(buf, sizeof(buf), "''");
            }
            break;
        case OP_LITERAL_BIN:
            snprintf(buf, sizeof(buf), "0x%X", op->value);
            break;
        default:
            snprintf(buf, sizeof(buf), "<?>");
            break;
    }

    if (op->indexed && op->index) {
        char *idx_str = operand_to_string(op->index);
        size_t result_len = strlen(buf) + strlen(idx_str) + 4;
        char *result = malloc(result_len);
        snprintf(result, result_len, "%s(%s)", buf, idx_str);
        free(idx_str);
        return result;
    }

    return strdup(buf);
}

/*
 * Check if a literal operand contains only numeric characters
 * (digits, optional leading minus). Used to determine if a string
 * literal like '123' should be displayed as unquoted 123.
 */
bool operand_is_numeric_literal(Operand *op) {
    if (!op) return false;
    if (op->kind == OP_LITERAL_NUM) return true;
    if (op->kind != OP_LITERAL_STR || !op->str_value || op->str_len == 0)
        return false;

    const char *s = op->str_value;
    int i = 0;
    if (s[i] == '-') i++;  /* allow leading minus */
    if (i >= op->str_len) return false;  /* just a minus sign */
    for (; i < op->str_len; i++) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

/*
 * Format operand as unquoted integer if it's a numeric literal string.
 */
char *operand_to_numeric_string(Operand *op) {
    if (operand_is_numeric_literal(op) && op->kind == OP_LITERAL_STR) {
        /* Return the raw string value without quotes */
        char *buf = malloc(op->str_len + 1);
        memcpy(buf, op->str_value, op->str_len);
        buf[op->str_len] = '\0';
        return buf;
    }
    return operand_to_string(op);
}

/*
 * Instruction functions
 */
Instruction *instruction_new(uint16_t address) {
    Instruction *instr = calloc(1, sizeof(Instruction));
    instr->address = address;
    return instr;
}

void instruction_free(Instruction *instr) {
    if (!instr) return;
    if (instr->operands) {
        for (int i = 0; i < instr->operand_count; i++) {
            operand_cleanup(&instr->operands[i]);
        }
        free(instr->operands);
    }
    free(instr);
}

void instruction_add_operand(Instruction *instr, Operand *op) {
    instr->operand_count++;
    Operand *tmp = realloc(instr->operands,
                           instr->operand_count * sizeof(Operand));
    if (!tmp) {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    instr->operands = tmp;
    instr->operands[instr->operand_count - 1] = *op;
    free(op);  /* Free the shell; contents were copied by value */
}

/*
 * Program functions
 */
Program *program_new(void) {
    return calloc(1, sizeof(Program));
}

void program_free(Program *prog) {
    if (!prog) return;
    if (prog->program_name) free(prog->program_name);
    if (prog->date_time) free(prog->date_time);
    if (prog->version) free(prog->version);
    if (prog->code) free(prog->code);

    Instruction *instr = prog->instructions;
    while (instr) {
        Instruction *next = instr->next;
        instruction_free(instr);
        instr = next;
    }

    free(prog);
}

/*
 * Expression tree functions
 */
Expr *expr_leaf_new(Instruction *instr) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = EXPR_LEAF;
    e->instr = instr;
    return e;
}

Expr *expr_binary_new(ExprKind kind, Expr *left, Expr *right) {
    Expr *e = calloc(1, sizeof(Expr));
    e->kind = kind;
    e->left = left;
    e->right = right;
    return e;
}

void expr_free(Expr *expr) {
    if (!expr) return;
    if (expr->kind != EXPR_LEAF) {
        expr_free(expr->left);
        expr_free(expr->right);
    }
    free(expr);
}

